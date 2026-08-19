#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../Data/State.hpp"
#include "../Common.hpp"
#include "../Audio/SoundManager.hpp"
#include <algorithm>
#include <vector>
#include <map>
#include <cmath>

using namespace geode::prelude;

class $modify(MyPlayLayer, PlayLayer) {
    struct Fields {
        int m_lastFrame = -1;                                   // 初始设为 -1，确保第 0 帧能被正确触发
        std::map<int, int> m_hudCounts;                         // 各预设 ID 对应的当前累计命中次数
        Ref<CCNode> m_hudNode = nullptr;                        // 左上角 HUD 容器节点
        Ref<CCNode> m_precNode = nullptr;                       // 左下角 Precision L* 容器节点
        int m_lastPrecIndex = -1;                               // 上一次渲染 L* 时所处的有效动作索引
        bool m_wasCalculating = false;                          // 记录上一帧是否处于后台计算中
        std::vector<Ref<CCNode>> m_activeMarkers;               // 场景中当前存活的标记节点列表
        std::map<int, Ref<CCLabelBMFont>> m_countLabels;        // 缓存各预设的数字标签以支持高性能局部文本刷新
    };

    bool init(GJGameLevel * level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        g_lastAutoSaveTime = std::chrono::steady_clock::now();
        loadModData();

        m_fields->m_lastFrame = -1;
        m_fields->m_lastPrecIndex = -1;
        m_fields->m_wasCalculating = false;
        m_fields->m_hudCounts.clear();
        m_fields->m_activeMarkers.clear();
        m_fields->m_countLabels.clear();

        this->rebuildHUD();
        this->schedule(schedule_selector(MyPlayLayer::onMyTick));

        return true;
    }

    void onQuit() {
        this->unschedule(schedule_selector(MyPlayLayer::onMyTick));
        SoundManager::stopAll();
        PlayLayer::onQuit();
    }

    void recalculateAndRefreshHUD() {
        if (!g_modEnabled) {
            if (m_fields->m_hudNode) {
                m_fields->m_hudNode->removeFromParent();
                m_fields->m_hudNode = nullptr;
                m_fields->m_countLabels.clear();
            }
            return;
        }

        m_fields->m_hudCounts.clear();
        for (const auto& action : g_tickActionsCache) {
            if (action.shouldDraw && action.frame < m_fields->m_lastFrame) {
                double fw = action.frameWindow;
                for (const auto& [idStr, preset] : g_labelPresets) {
                    if (fw >= preset.minVal && fw <= preset.maxVal) {
                        m_fields->m_hudCounts[preset.id]++;
                    }
                }
            }
        }
        this->rebuildHUD();
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        SoundManager::stopAll();

        int currentFrame = static_cast<int>(this->m_gameState.m_levelTime * g_macroFps);
        m_fields->m_lastFrame = currentFrame;
        m_fields->m_lastPrecIndex = -1;
        m_fields->m_hudCounts.clear();

        // 清空存活标记
        for (auto& marker : m_fields->m_activeMarkers) {
            if (marker) marker->removeFromParent();
        }
        m_fields->m_activeMarkers.clear();

        // 重新统计复活点之前的 HUD 数据
        for (const auto& action : g_tickActionsCache) {
            if (action.shouldDraw && action.frame < currentFrame) {
                double fw = action.frameWindow;
                for (const auto& [idStr, preset] : g_labelPresets) {
                    if (fw >= preset.minVal && fw <= preset.maxVal) {
                        m_fields->m_hudCounts[preset.id]++;
                    }
                }
            }
        }

        this->updateHUDCounts();
        this->updatePrecisionHUD(currentFrame);

        if (this->m_objectLayer) {
            auto children = this->m_objectLayer->getChildren();
            if (children) {
                for (int i = children->count() - 1; i >= 0; i--) {
                    auto child = static_cast<CCNode*>(children->objectAtIndex(i));
                    if (child->getID() == "frame-window-marker"_spr) {
                        child->removeFromParent();
                    }
                }
            }
        }
    }

    void updatePrecisionHUD(int currentFrame) {
        if (!g_modEnabled) {
            if (m_fields->m_precNode) {
                m_fields->m_precNode->removeFromParent();
                m_fields->m_precNode = nullptr;
            }
            return;
        }

        bool isCalc = g_isCalculating.load();

        if (isCalc) {
            m_fields->m_wasCalculating = true;
            if (!m_fields->m_precNode || g_forcePrecRedraw) {
                g_forcePrecRedraw = false;

                if (m_fields->m_precNode) m_fields->m_precNode->removeFromParent();
                m_fields->m_precNode = CCNode::create();
                m_fields->m_precNode->setPosition({ 5.f, 5.f });
                m_fields->m_precNode->setID("frame-window-prec"_spr);

                if (this->m_uiLayer) this->m_uiLayer->addChild(m_fields->m_precNode, 9999);
                else this->addChild(m_fields->m_precNode, 9999);

                auto lbl = CCLabelBMFont::create("Calculating L*... 0.0%", "bigFont.fnt");
                lbl->setAnchorPoint({ 0.f, 0.f });
                lbl->setPosition({ 0.f, 0.f });
                lbl->setScale(0.35f);
                lbl->setColor({ 255, 200, 50 });
                lbl->setID("computing-label"_spr);
                m_fields->m_precNode->addChild(lbl);
            }
            else {
                if (auto lbl = static_cast<CCLabelBMFont*>(m_fields->m_precNode->getChildByID("computing-label"_spr))) {
                    lbl->setString(fmt::format("Calculating L*... {:.1f}%", g_calcProgress.load()).c_str());
                }
            }
            return;
        }

        if (m_fields->m_wasCalculating) {
            m_fields->m_wasCalculating = false;
            g_forcePrecRedraw = true;
            m_fields->m_lastPrecIndex = -999;
        }

        if (g_validActions.empty()) {
            if (m_fields->m_precNode) {
                m_fields->m_precNode->removeFromParent();
                m_fields->m_precNode = nullptr;
            }
            return;
        }

        int idx = -1;
        auto it = std::upper_bound(g_validActions.begin(), g_validActions.end(), currentFrame,
            [](int frame, const FrameAction& a) { return frame < a.frame; });
        if (it != g_validActions.begin()) {
            idx = static_cast<int>(std::distance(g_validActions.begin(), it) - 1);
        }

        if (idx != m_fields->m_lastPrecIndex || !m_fields->m_precNode || g_forcePrecRedraw) {
            m_fields->m_lastPrecIndex = idx;
            g_forcePrecRedraw = false;

            if (m_fields->m_precNode) m_fields->m_precNode->removeFromParent();
            m_fields->m_precNode = CCNode::create();
            m_fields->m_precNode->setPosition({ 5.f, 5.f });
            m_fields->m_precNode->setID("frame-window-prec"_spr);

            if (this->m_uiLayer) this->m_uiLayer->addChild(m_fields->m_precNode, 9999);
            else this->addChild(m_fields->m_precNode, 9999);

            float currentY = 0.f;
            auto addLabel = [&](bool show, const char* prefix, const std::vector<double>& arr) {
                if (!show) return;
                std::string text = prefix;
                if (idx >= 0 && idx < static_cast<int>(arr.size())) {
                    text += fmt::format("{:.2f}", arr[idx]);
                }
                else if (!arr.empty()) {
                    text += fmt::format("{:.2f}", arr[0]);
                }
                else {
                    text += "0.00";
                }

                auto lbl = CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
                lbl->setAnchorPoint({ 0.f, 0.f });
                lbl->setPosition({ 0.f, currentY });
                lbl->setScale(0.3f);
                m_fields->m_precNode->addChild(lbl);
                currentY += 12.f;
                };

            addLabel(g_showNFC, "Nerve+Fatigue+CPS L*: ", g_vNFC);
            addLabel(g_showFC, "Fatigue+CPS L*: ", g_vFC);
            addLabel(g_showNC, "Nerve+CPS L*: ", g_vNC);
            addLabel(g_showNF, "Nerve+Fatigue L*: ", g_vNF);
            addLabel(g_showC, "CPS L*: ", g_vC);
            addLabel(g_showF, "Fatigue L*: ", g_vF);
            addLabel(g_showN, "Nerve L*: ", g_vN);
            addLabel(g_showBase, "L*: ", g_vBase);
        }
    }

    void rebuildHUD() {
        if (m_fields->m_hudNode) {
            m_fields->m_hudNode->removeFromParent();
            m_fields->m_hudNode = nullptr;
        }
        m_fields->m_countLabels.clear();

        if (!g_modEnabled) return;

        m_fields->m_hudNode = CCNode::create();
        m_fields->m_hudNode->setPosition({ 5.f, CCDirector::get()->getWinSize().height - 5.f });
        m_fields->m_hudNode->setID("frame-window-hud"_spr);

        if (this->m_uiLayer) this->m_uiLayer->addChild(m_fields->m_hudNode, 9999);
        else this->addChild(m_fields->m_hudNode, 9999);

        struct HudItem {
            int id;
            CCLabelBMFont* textLbl;
            CCLabelBMFont* countLbl;
        };
        std::vector<HudItem> items;
        float maxTextWidth = 25.f;

        for (int i = 99; i >= 0; i--) {
            std::string idStr = std::to_string(i);
            if (!g_labelPresets.contains(idStr)) continue;
            auto& preset = g_labelPresets[idStr];
            if (!preset.showInHud) continue;

            int count = m_fields->m_hudCounts.contains(i) ? m_fields->m_hudCounts[i] : 0;
            auto textLabel = CCLabelBMFont::create((preset.text + ":").c_str(), "bigFont.fnt");
            textLabel->setAnchorPoint({ 0.f, 1.f });
            textLabel->setScale(0.5f);
            textLabel->setColor({
                static_cast<GLubyte>(preset.color.r * 255),
                static_cast<GLubyte>(preset.color.g * 255),
                static_cast<GLubyte>(preset.color.b * 255)
                });
            textLabel->setOpacity(static_cast<GLubyte>(preset.color.a * 255));

            float currentWidth = textLabel->getScaledContentSize().width;
            if (currentWidth > maxTextWidth) maxTextWidth = currentWidth;

            auto countLabel = CCLabelBMFont::create(std::to_string(count).c_str(), "bigFont.fnt");
            countLabel->setAnchorPoint({ 0.f, 1.f });
            countLabel->setScale(0.5f);
            countLabel->setColor({
                static_cast<GLubyte>(preset.color.r * 255),
                static_cast<GLubyte>(preset.color.g * 255),
                static_cast<GLubyte>(preset.color.b * 255)
                });
            countLabel->setOpacity(static_cast<GLubyte>(preset.color.a * 255));

            items.push_back({ i, textLabel, countLabel });
        }

        float currentY = -10.f;
        for (auto& item : items) {
            item.textLbl->setPosition({ 0.f, currentY });
            m_fields->m_hudNode->addChild(item.textLbl);

            item.countLbl->setPosition({ maxTextWidth + 3.f, currentY });
            m_fields->m_hudNode->addChild(item.countLbl);

            m_fields->m_countLabels[item.id] = item.countLbl;
            currentY -= 18.f;
        }
    }

    void updateHUDCounts() {
        if (!g_modEnabled) {
            if (m_fields->m_hudNode) {
                m_fields->m_hudNode->removeFromParent();
                m_fields->m_hudNode = nullptr;
                m_fields->m_countLabels.clear();
            }
            return;
        }

        if (!m_fields->m_hudNode) {
            this->rebuildHUD();
            return;
        }

        for (auto& [id, countLbl] : m_fields->m_countLabels) {
            if (countLbl) {
                int count = m_fields->m_hudCounts.contains(id) ? m_fields->m_hudCounts[id] : 0;
                countLbl->setString(std::to_string(count).c_str());
            }
        }
    }

    void cleanupOffscreenMarkers() {
        if (!this->m_objectLayer) return;

        CCSize winSize = CCDirector::get()->getWinSize();

        constexpr float margin = 300.0f;
        float minX = -margin;
        float maxX = winSize.width + margin;
        float minY = -margin;
        float maxY = winSize.height + margin;

        for (auto it = m_fields->m_activeMarkers.begin(); it != m_fields->m_activeMarkers.end(); ) {
            auto& marker = *it;
            if (!marker || !marker->getParent()) {
                it = m_fields->m_activeMarkers.erase(it);
                continue;
            }

            CCPoint screenPos = this->m_objectLayer->convertToWorldSpace(marker->getPosition());

            if (screenPos.x < minX || screenPos.x > maxX || screenPos.y < minY || screenPos.y > maxY) {
                marker->removeFromParent();
                it = m_fields->m_activeMarkers.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void onMyTick(float dt) {
        if (!g_modEnabled) return;
        auto gm = GameManager::sharedState();
        if (gm->getPlayLayer() && !gm->getPlayLayer()->m_isPaused) {
            if (this->m_player1 && !this->m_player1->m_isDead) {
                int currentFrame = static_cast<int>(this->m_gameState.m_levelTime * g_macroFps);
                this->updatePrecisionHUD(currentFrame);

                // 1. 真正的时间倒退（读档、练习模式重试、死亡回退）
                if (currentFrame < m_fields->m_lastFrame) {
                    m_fields->m_lastFrame = currentFrame;
                    m_fields->m_hudCounts.clear();
                    SoundManager::stopAll();

                    for (auto& marker : m_fields->m_activeMarkers) {
                        if (marker) marker->removeFromParent();
                    }
                    m_fields->m_activeMarkers.clear();

                    for (const auto& action : g_tickActionsCache) {
                        if (action.shouldDraw && action.frame < currentFrame) {
                            double fw = action.frameWindow;
                            for (const auto& [idStr, preset] : g_labelPresets) {
                                if (fw >= preset.minVal && fw <= preset.maxVal) {
                                    m_fields->m_hudCounts[preset.id]++;
                                }
                            }
                        }
                    }
                    this->updateHUDCounts();
                }
                // 2. 正常时间向前推进：处理 (m_lastFrame, currentFrame] 区间
                else if (currentFrame > m_fields->m_lastFrame) {
                    bool skipAudio = (currentFrame - m_fields->m_lastFrame > static_cast<int>(g_macroFps));
                    bool needsHudUpdate = false;

                    auto it = std::upper_bound(g_tickActionsCache.begin(), g_tickActionsCache.end(), m_fields->m_lastFrame,
                        [](int frame, const FrameAction& a) { return frame < a.frame; });

                    while (it != g_tickActionsCache.end() && it->frame <= currentFrame) {
                        auto& action = *it;
                        if (action.shouldDraw) {
                            double fw = action.frameWindow;
                            ccColor4F markerColor = { 1.f, 1.f, 1.f, 1.f };
                            std::string fwStr = formatWindowVal(fw);
                            if (g_windowPresets.contains(fwStr)) {
                                markerColor = g_windowPresets[fwStr].color;
                            }

                            CCPoint spawnPos = this->m_player1->getPosition();
                            if (action.isPlayer2 && this->m_player2) {
                                spawnPos = this->m_player2->getPosition();
                            }
                            this->spawnFrameWindowMarker(spawnPos, fw, markerColor);

                            for (auto& [idStr, preset] : g_labelPresets) {
                                if (fw >= preset.minVal && fw <= preset.maxVal) {
                                    if (!skipAudio && !preset.audioPath.empty() && preset.showInHud) {
                                        SoundManager::playSound(preset.audioPath);
                                    }
                                    m_fields->m_hudCounts[preset.id]++;
                                    if (preset.showInHud) {
                                        needsHudUpdate = true;
                                    }
                                }
                            }
                        }
                        it++;
                    }

                    m_fields->m_lastFrame = currentFrame;
                    if (needsHudUpdate) this->updateHUDCounts();

                    this->cleanupOffscreenMarkers();
                }
            }
        }
    }

    void spawnFrameWindowMarker(CCPoint pos, double frameWindow, ccColor4F color) {
        auto markerNode = CCNode::create();
        markerNode->setPosition(pos);
        markerNode->setZOrder(9999);
        markerNode->setID("frame-window-marker"_spr);

        auto circle = CCDrawNode::create();
        CCPoint verts[64];
        float radius = 13.f;
        for (int i = 0; i < 64; i++) {
            float angle = i * (3.14159f * 2.f) / 64.f;
            verts[i] = CCPoint{ radius * cosf(angle), radius * sinf(angle) };
        }

        float a = color.a;
        float r = color.r * a;
        float g = color.g * a;
        float b = color.b * a;

        // 绘制黑色描边底圈与主体颜色圈
        circle->drawPolygon(verts, 64, { 0.f, 0.f, 0.f, 0.f }, 4.f, { 0.f, 0.f, 0.f, color.a });
        circle->drawPolygon(verts, 64, { 0.f, 0.f, 0.f, 0.f }, 2.f, { r, g, b, a });
        markerNode->addChild(circle);

        std::string labelStr = (std::abs(frameWindow) < 1e-6) ? "S" : formatWindowVal(frameWindow);
        auto label = CCLabelBMFont::create(labelStr.c_str(), "bigFont.fnt");
        label->setAnchorPoint({ 1.f, 0.5f });
        label->setPosition({ -18.f, 0.f });
        label->setScale(0.5f);
        label->setColor({
            static_cast<GLubyte>(color.r * 255),
            static_cast<GLubyte>(color.g * 255),
            static_cast<GLubyte>(color.b * 255)
            });
        label->setOpacity(static_cast<GLubyte>(color.a * 255));
        markerNode->addChild(label);

        this->m_objectLayer->addChild(markerNode);
        m_fields->m_activeMarkers.push_back(markerNode);
    }
};

void triggerHUDRefresh() {
    if (auto pl = PlayLayer::get()) {
        static_cast<MyPlayLayer*>(pl)->recalculateAndRefreshHUD();
    }
}