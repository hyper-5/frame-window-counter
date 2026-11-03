#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/binding/CheckpointObject.hpp>
#include "../Data/State.hpp"
#include "../Common.hpp"
#include "../Audio/SoundManager.hpp"
#include "../UI/FrameActionPopup.hpp"
#include <algorithm>
#include <vector>
#include <map>
#include <cmath>
#include <cstddef>

using namespace geode::prelude;

struct ActiveMarker {
    Ref<CCNode> node;
    CCPoint worldPos;
};

class $modify(MyPlayLayer, PlayLayer) {
    struct Fields {
        int m_currentFrame = 0;                                 // 基于实际物理步计数的帧号，不受 timewarp 干扰
        int m_lastFrame = -1;                                   // 初始设为 -1，保证第 0 帧能够触发
        std::map<void*, int> m_checkpointMap;                   // 记录检查点对象对应的首次保存帧号，仅用于去重与普通加载
        std::vector<int> m_checkpointFrameStack;                // 按检查点栈顺序独立保存每个检查点的真实物理帧号
		int m_pendingRestoreFrame = -1;                         // 记录下一次 restoreCheckpoint() 的目标帧号，-1 表示无效
		bool m_restoreFrameArmed = false;                       // 标记下一次 restoreCheckpoint() 是否应当执行帧恢复
		bool m_checkpointRemovalActive = false;                 // 标记当前是否处于 removeCheckpoint() 的帧恢复阶段
		int m_checkpointRemovalRestoreFrame = -1;               // 记录 removeCheckpoint() 触发的帧恢复目标帧号，-1 表示无效
        std::map<int, int> m_hudCounts;                         // 各预设 ID 对应的当前累计命中次数
        Ref<CCNode> m_hudNode = nullptr;                        // 左上角 HUD 容器节点
        Ref<CCNode> m_precNode = nullptr;                       // 左下角 Precision L* 容器节点
        int m_lastPrecIndex = -1;                               // 上一次渲染 L* 时所处的有效动作索引
        bool m_wasCalculating = false;                          // 记录上一帧是否处于后台计算中
        std::vector<ActiveMarker> m_activeMarkers;              // 记录标记节点及其原始世界坐标
        std::map<int, Ref<CCLabelBMFont>> m_countLabels;        // 缓存各预设的数字标签以支持高性能局部文本刷新

        // 1P 与 2P 分立的最近画圈帧号记录
        int m_lastSpawnFrame1P = -1;
        int m_lastSpawnFrame2P = -1;

#if defined(GEODE_IS_MOBILE)
        bool m_isLevelEnd = false;                              // 标记关卡是否已触碰终点
#endif
    };

    bool init(GJGameLevel * level, bool dontCreateObjects) {
        if (!PlayLayer::init(level, dontCreateObjects)) return false;

        g_lastAutoSaveTime = std::chrono::steady_clock::now();
        loadModData();

        m_fields->m_currentFrame = 0;
        m_fields->m_lastFrame = -1;
        m_fields->m_checkpointMap.clear();
        m_fields->m_checkpointFrameStack.clear();
        m_fields->m_pendingRestoreFrame = -1;
        m_fields->m_restoreFrameArmed = false;
        m_fields->m_checkpointRemovalActive = false;
        m_fields->m_checkpointRemovalRestoreFrame = -1;
        m_fields->m_lastPrecIndex = -1;
        m_fields->m_wasCalculating = false;
        m_fields->m_hudCounts.clear();
        m_fields->m_activeMarkers.clear();
        m_fields->m_countLabels.clear();
        m_fields->m_lastSpawnFrame1P = -1;
        m_fields->m_lastSpawnFrame2P = -1;

#if defined(GEODE_IS_MOBILE)
        m_fields->m_isLevelEnd = false;
        this->createMobileShortcutBtn();
#endif

        this->rebuildHUD();
        this->schedule(schedule_selector(MyPlayLayer::onMyTick));

        return true;
    }

    void rememberCheckpoint(CheckpointObject * cp) {
        if (!cp) return;

        auto key = static_cast<void*>(cp);

        if (m_fields->m_checkpointMap.contains(key)) {
            return;
        }

        const int frame = m_fields->m_currentFrame;
        m_fields->m_checkpointMap[key] = frame;
        m_fields->m_checkpointFrameStack.push_back(frame);

        geode::log::debug(
            "FrameAction: checkpoint pushed at physical frame {} ({:p}), stack size {}",
            frame, key, m_fields->m_checkpointFrameStack.size()
        );
    }

    int getSavedCheckpointFrame(CheckpointObject * cp) {
        if (cp && this->m_checkpointArray) {
            for (unsigned int i = 0; i < this->m_checkpointArray->count(); i++) {
                auto currentCp = static_cast<CheckpointObject*>(
                    this->m_checkpointArray->objectAtIndex(i)
                    );

                if (currentCp == cp) {
                    if (i < m_fields->m_checkpointFrameStack.size()) {
                        return m_fields->m_checkpointFrameStack[i];
                    }
                    break;
                }
            }
        }

        if (cp) {
            auto it = m_fields->m_checkpointMap.find(static_cast<void*>(cp));
            if (it != m_fields->m_checkpointMap.end()) {
                return it->second;
            }
        }

        if (!m_fields->m_checkpointFrameStack.empty()) {
            return m_fields->m_checkpointFrameStack.back();
        }

        return -1;
    }

    // 放置检查点时，记录当前真实的物理帧号
    CheckpointObject* createCheckpoint() {
        auto cp = PlayLayer::createCheckpoint();

        if (cp) {
            rememberCheckpoint(cp);
        }

        return cp;
    }

    void storeCheckpoint(CheckpointObject * cp) {
        PlayLayer::storeCheckpoint(cp);
    }

    // 移除检查点时，从独立 checkpoint 栈中弹出最上层，
    // 并把新的栈顶作为下一次恢复目标。
    void removeCheckpoint(bool p0) {
        if (this->m_isPracticeMode &&
            !m_fields->m_checkpointFrameStack.empty()) {

            const int beforeStackSize =
                static_cast<int>(m_fields->m_checkpointFrameStack.size());

            if (!p0 || beforeStackSize == 1) {
                m_fields->m_checkpointFrameStack.pop_back();

                if (!m_fields->m_checkpointFrameStack.empty()) {
                    m_fields->m_checkpointRemovalActive = true;
                    m_fields->m_checkpointRemovalRestoreFrame =
                        m_fields->m_checkpointFrameStack.back();
                    armCheckpointRestore(
                        m_fields->m_checkpointFrameStack.back()
                    );
                }
                else {
                    m_fields->m_checkpointRemovalActive = false;
                    m_fields->m_checkpointRemovalRestoreFrame = -1;
                    m_fields->m_pendingRestoreFrame = -1;
                    m_fields->m_restoreFrameArmed = false;
                }
            }
        }

        PlayLayer::removeCheckpoint(p0);

        m_fields->m_checkpointMap.clear();

        if (this->m_checkpointArray) {
            const unsigned int count = this->m_checkpointArray->count();
            const unsigned int stackCount =
                static_cast<unsigned int>(m_fields->m_checkpointFrameStack.size());

            for (unsigned int i = 0; i < count && i < stackCount; i++) {
                auto cp = static_cast<CheckpointObject*>(
                    this->m_checkpointArray->objectAtIndex(i)
                    );

                if (cp) {
                    m_fields->m_checkpointMap[static_cast<void*>(cp)] =
                        m_fields->m_checkpointFrameStack[i];
                }
            }
        }

        // 如果当前删除流程已经结束，确保恢复目标始终是新的栈顶。
        if (m_fields->m_checkpointRemovalActive &&
            !m_fields->m_checkpointFrameStack.empty()) {
            m_fields->m_checkpointRemovalRestoreFrame =
                m_fields->m_checkpointFrameStack.back();
            armCheckpointRestore(
                m_fields->m_checkpointFrameStack.back()
            );
        }
    }

    void armCheckpointRestore(int frame) {
        if (frame < 0) return;

        m_fields->m_pendingRestoreFrame = frame;
        m_fields->m_restoreFrameArmed = true;
    }

    void applyRestoredFrame() {
        if (!m_fields->m_restoreFrameArmed) return;

        const int frame = m_fields->m_pendingRestoreFrame;
        if (frame < 0) {
            m_fields->m_restoreFrameArmed = false;
            return;
        }

        m_fields->m_currentFrame = frame;
        m_fields->m_lastFrame = frame - 1;
        m_fields->m_lastPrecIndex = -1;
        g_forcePrecRedraw = true;
        m_fields->m_hudCounts.clear();
        m_fields->m_lastSpawnFrame1P = -1;
        m_fields->m_lastSpawnFrame2P = -1;

        geode::log::debug(
            "FrameAction: restored physical frame {}",
            frame
        );

        m_fields->m_restoreFrameArmed = false;
        m_fields->m_pendingRestoreFrame = -1;
    }

    // 从检查点恢复时，恢复保存的真实物理帧号
    void loadFromCheckpoint(CheckpointObject * p0) {
        int checkpointFrame = -1;

        if (m_fields->m_checkpointRemovalActive) {
            checkpointFrame = m_fields->m_checkpointRemovalRestoreFrame;
        }
        else {
            checkpointFrame = getSavedCheckpointFrame(p0);
        }

        if (checkpointFrame >= 0) {
            armCheckpointRestore(checkpointFrame);
        }

        PlayLayer::loadFromCheckpoint(p0);

        applyRestoredFrame();
    }

    void processCheckpoints() {
        PlayLayer::processCheckpoints();

        if (m_fields->m_checkpointRemovalActive) {
            armCheckpointRestore(m_fields->m_checkpointRemovalRestoreFrame);
            applyRestoredFrame();
        }
        else if (m_fields->m_restoreFrameArmed) {
            applyRestoredFrame();
        }
        else if (this->m_isPracticeMode &&
            m_fields->m_currentFrame == 0 &&
            !m_fields->m_checkpointFrameStack.empty()) {
            armCheckpointRestore(m_fields->m_checkpointFrameStack.back());
            applyRestoredFrame();
        }
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        if (m_fields->m_checkpointRemovalActive) {
            armCheckpointRestore(m_fields->m_checkpointRemovalRestoreFrame);
            applyRestoredFrame();
            m_fields->m_checkpointRemovalActive = false;
            m_fields->m_checkpointRemovalRestoreFrame = -1;
        }
        else if (m_fields->m_restoreFrameArmed) {
            applyRestoredFrame();
        }
    }

#if defined(GEODE_IS_MOBILE)
    // 创建移动端快捷入口按钮
    void createMobileShortcutBtn() {
        if (!this->m_uiLayer) return;

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        // 依次尝试获取合适的图标
        auto sprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn02_001.png");
        if (!sprite) {
            sprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        }
        if (!sprite) {
            sprite = CCSprite::createWithSpriteFrameName("GJ_menuBtn_001.png");
        }

        sprite->setScale(0.6f);
        sprite->setOpacity(180);

        auto btn = CCMenuItemSpriteExtra::create(
            sprite,
            this,
            menu_selector(MyPlayLayer::onOpenModMenu)
        );
        btn->setID("mobile-shortcut-btn"_spr);

        auto menu = CCMenu::create();
        menu->setID("mobile-shortcut-menu"_spr);
        menu->setZOrder(100);
        menu->setPosition(winSize.width - 32.f, 32.f);
        menu->addChild(btn);

        this->m_uiLayer->addChild(menu);
    }

    // 点击按钮打开/切换 Mod 窗口
    void onOpenModMenu(CCObject*) {
        // 如果关卡已经完成/正在播放通关动画，禁止再弹出 Mod
        if (m_fields->m_isLevelEnd) {
            return;
        }

        auto scene = CCDirector::sharedDirector()->getRunningScene();
        if (!scene) return;

        // 如果弹窗已打开，再次按下则关闭；否则打开主弹窗
        if (auto existing = scene->getChildByID("FrameActionPopup"_spr)) {
            existing->removeFromParentAndCleanup(true);
            return;
        }

        if (auto popup = FrameActionPopup::create()) {
            popup->setID("FrameActionPopup"_spr);
            popup->showInstant();
        }
    }

    // 切后台或手动暂停时关闭所有弹窗
    void pauseGame(bool p0) {
        closeAllModPopups();
        PlayLayer::pauseGame(p0);
    }

    // 触碰终点通关时关闭所有弹窗并加锁
    void levelComplete() {
        m_fields->m_isLevelEnd = true;
        closeAllModPopups();
        PlayLayer::levelComplete();
    }
#endif

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
            if (action.shouldDraw && action.frame <= m_fields->m_lastFrame) {
                double fw = action.frameWindow;
                double ifVal = static_cast<double>(action.ifCount);
                for (const auto& [idStr, preset] : g_labelPresets) {
                    double targetVal = preset.useIF ? ifVal : fw;
                    if (targetVal >= preset.minVal && targetVal <= preset.maxVal) {
                        m_fields->m_hudCounts[preset.id]++;
                    }
                }
            }
        }
        this->rebuildHUD();
    }

    void resetLevel() {
        int restoreFrame = -1;

        if (m_fields->m_checkpointRemovalActive &&
            m_fields->m_checkpointRemovalRestoreFrame >= 0) {
            restoreFrame = m_fields->m_checkpointRemovalRestoreFrame;
        }
        else if (m_fields->m_pendingRestoreFrame >= 0) {
            restoreFrame = m_fields->m_pendingRestoreFrame;
        }
        else if (this->m_isPracticeMode) {
            CheckpointObject* cp = this->m_currentCheckpoint;

            if (!cp && this->m_checkpointArray &&
                this->m_checkpointArray->count() > 0) {
                cp = static_cast<CheckpointObject*>(
                    this->m_checkpointArray->lastObject()
                    );
            }

            if (cp) {
                restoreFrame = getSavedCheckpointFrame(cp);
            }

            if (restoreFrame < 0 &&
                !m_fields->m_checkpointFrameStack.empty()) {
                restoreFrame = m_fields->m_checkpointFrameStack.back();
            }
        }

        if (restoreFrame >= 0) {
            armCheckpointRestore(restoreFrame);
        }

        PlayLayer::resetLevel();
        SoundManager::stopAll();

#if defined(GEODE_IS_MOBILE)
        m_fields->m_isLevelEnd = false; // 复活时重置通关锁
#endif

        if (m_fields->m_restoreFrameArmed) {
            applyRestoredFrame();
        }
        else if (restoreFrame >= 0) {
            m_fields->m_currentFrame = restoreFrame;
            m_fields->m_lastFrame = restoreFrame - 1; // 设为前一帧，保证当前起点帧的动作能在 onPhysicsStep 中触发
            m_fields->m_lastPrecIndex = -1;
            g_forcePrecRedraw = true;
            m_fields->m_hudCounts.clear();
            m_fields->m_lastSpawnFrame1P = -1;
            m_fields->m_lastSpawnFrame2P = -1;
        }
        else {
            m_fields->m_currentFrame = 0;
            m_fields->m_lastFrame = -1;
            m_fields->m_lastPrecIndex = -1;
            g_forcePrecRedraw = true;
            m_fields->m_hudCounts.clear();
            m_fields->m_lastSpawnFrame1P = -1;
            m_fields->m_lastSpawnFrame2P = -1;
        }

        // 清空存活标记
        for (auto& marker : m_fields->m_activeMarkers) {
            if (marker.node) marker.node->removeFromParent();
        }
        m_fields->m_activeMarkers.clear();

        // 重新统计复活起点之前的 HUD 数据
        for (const auto& action : g_tickActionsCache) {
            if (action.shouldDraw && action.frame <= m_fields->m_lastFrame) {
                double fw = action.frameWindow;
                double ifVal = static_cast<double>(action.ifCount);
                for (const auto& [idStr, preset] : g_labelPresets) {
                    double targetVal = preset.useIF ? ifVal : fw;
                    if (targetVal >= preset.minVal && targetVal <= preset.maxVal) {
                        m_fields->m_hudCounts[preset.id]++;
                    }
                }
            }
        }

        this->updateHUDCounts();
        this->updatePrecisionHUD(m_fields->m_currentFrame);

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

                if (this->m_uiLayer) this->m_uiLayer->addChild(m_fields->m_precNode, 1);
                else this->addChild(m_fields->m_precNode, 1);

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
            m_fields->m_lastPrecIndex = -1;
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

            if (this->m_uiLayer) this->m_uiLayer->addChild(m_fields->m_precNode, 1);
            else this->addChild(m_fields->m_precNode, 1);

            float currentY = 0.f;
            auto addLabel = [&](bool show, const char* prefix, const std::vector<double>& arr) {
                if (!show) return;
                std::string text = prefix;
                if (idx >= 0 && idx < static_cast<int>(arr.size())) {
                    text += fmt::format("{:.2f}", arr[idx]);
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

    void playHUDCountAnimation(int presetId) {
        auto it = m_fields->m_countLabels.find(presetId);
        if (it == m_fields->m_countLabels.end() || !it->second) return;

        auto countLbl = it->second;

        std::string idStr = std::to_string(presetId);
        ccColor3B origColor = { 255, 255, 255 };
        if (g_labelPresets.contains(idStr)) {
            auto& p = g_labelPresets[idStr];
            origColor = {
                static_cast<GLubyte>(p.color.r * 255),
                static_cast<GLubyte>(p.color.g * 255),
                static_cast<GLubyte>(p.color.b * 255)
            };
        }

        // 清理残留的旧 Glow 节点
        const int TAG_GLOW_NODE = 20000 + presetId;
        if (m_fields->m_hudNode) {
            if (auto oldGlow = m_fields->m_hudNode->getChildByTag(TAG_GLOW_NODE)) {
                oldGlow->removeFromParentAndCleanup(true);
            }
        }

        constexpr int TAG_SCALE = 1001;
        constexpr int TAG_TINT = 1002;

        countLbl->stopActionByTag(TAG_SCALE);
        countLbl->stopActionByTag(TAG_TINT);

        // 动画时长与幅度配置
        constexpr float TIME_UP = 0.06f; // 放大并变白的时长
        constexpr float TIME_DOWN = 0.20f; // 缩回并恢复原色的时长
        constexpr float BASE_SCALE = 0.50f; // 常态尺寸
        constexpr float PEAK_SCALE = 0.60f; // 弹起峰值尺寸

        // 缩放动画
        auto scaleUp = CCEaseSineOut::create(CCScaleTo::create(TIME_UP, PEAK_SCALE));
        auto scaleDown = CCEaseSineOut::create(CCScaleTo::create(TIME_DOWN, BASE_SCALE));
        auto scaleSeq = CCSequence::create(scaleUp, scaleDown, nullptr);
        scaleSeq->setTag(TAG_SCALE);
        countLbl->runAction(scaleSeq);

        // 颜色渐变
        auto tintToWhite = CCEaseSineOut::create(CCTintTo::create(TIME_UP, 255, 255, 255));
        auto tintToOrig = CCEaseSineOut::create(CCTintTo::create(TIME_DOWN, origColor.r, origColor.g, origColor.b));
        auto tintSeq = CCSequence::create(tintToWhite, tintToOrig, nullptr);
        tintSeq->setTag(TAG_TINT);
        countLbl->runAction(tintSeq);
    }

    void updateAndCleanMarkers() {
        if (!this->m_objectLayer) return;

        CCSize winSize = CCDirector::get()->getWinSize();
        float layerScale = std::abs(this->m_objectLayer->getScaleY());

        constexpr float margin = 300.0f;
        float minX = -margin;
        float maxX = winSize.width + margin;
        float minY = -margin;
        float maxY = winSize.height + margin;

        for (auto it = m_fields->m_activeMarkers.begin(); it != m_fields->m_activeMarkers.end(); ) {
            auto& marker = *it;
            if (!marker.node || !marker.node->getParent()) {
                it = m_fields->m_activeMarkers.erase(it);
                continue;
            }

            // 将物体层的世界坐标转换为屏幕坐标
            CCPoint screenPos = this->m_objectLayer->convertToWorldSpace(marker.worldPos);
            marker.node->setPosition(screenPos);
            marker.node->setScale(layerScale); // 随摄像机缩放同步变化

            // 越界清理
            if (screenPos.x < minX || screenPos.x > maxX || screenPos.y < minY || screenPos.y > maxY) {
                marker.node->removeFromParent();
                it = m_fields->m_activeMarkers.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void onPhysicsStep() {
        if (!g_modEnabled) return;
        if (!this->m_player1 || this->m_player1->m_isDead) return;

        if (m_fields->m_restoreFrameArmed) {
            applyRestoredFrame();
        }

        m_fields->m_currentFrame++;
        int currentFrame = m_fields->m_currentFrame;

        if (currentFrame < m_fields->m_lastFrame) {
            m_fields->m_lastFrame = currentFrame - 1;
            m_fields->m_lastPrecIndex = -1;
            g_forcePrecRedraw = true;
            m_fields->m_hudCounts.clear();
            m_fields->m_lastSpawnFrame1P = -1;
            m_fields->m_lastSpawnFrame2P = -1;
            SoundManager::stopAll();

            for (auto& marker : m_fields->m_activeMarkers) {
                if (marker.node) marker.node->removeFromParent();
            }
            m_fields->m_activeMarkers.clear();

            for (const auto& action : g_tickActionsCache) {
                if (action.shouldDraw && action.frame <= m_fields->m_lastFrame) {
                    double fw = action.frameWindow;
                    double ifVal = static_cast<double>(action.ifCount);
                    for (const auto& [idStr, preset] : g_labelPresets) {
                        double targetVal = preset.useIF ? ifVal : fw;
                        if (targetVal >= preset.minVal && targetVal <= preset.maxVal) {
                            m_fields->m_hudCounts[preset.id]++;
                        }
                    }
                }
            }
            this->updateHUDCounts();
        }

        if (currentFrame > m_fields->m_lastFrame) {
            bool skipAudio = (currentFrame - m_fields->m_lastFrame > static_cast<int>(g_macroFps));
            bool needsHudUpdate = false;
            std::vector<int> updatedPresets;

            auto it = std::upper_bound(g_tickActionsCache.begin(), g_tickActionsCache.end(), m_fields->m_lastFrame,
                [](int frame, const FrameAction& a) { return frame < a.frame; });

            while (it != g_tickActionsCache.end() && it->frame <= currentFrame) {
                auto& action = *it;
                if (action.shouldDraw) {
                    double fw = action.frameWindow;
                    int ifVal = action.ifCount;
                    ccColor4F markerColor = { 1.f, 1.f, 1.f, 1.f };

                    std::string markerText = formatWindowVal(fw);
                    if (ifVal > 1) {
                        markerText += fmt::format(" ({}I/F)", ifVal);
                    }

                    std::string presetKey = makeWindowPresetKey(ifVal, fw);
                    if (g_windowPresets.contains(presetKey)) {
                        auto& preset = g_windowPresets[presetKey];
                        markerColor = preset.color;
                        if (!preset.customText.empty()) {
                            markerText = preset.customText;
                        }
                    }

                    bool shouldSpawnMarker = false;
                    if (!action.isPlayer2) {
                        if (action.frame != m_fields->m_lastSpawnFrame1P) {
                            shouldSpawnMarker = true;
                            m_fields->m_lastSpawnFrame1P = action.frame;
                        }
                    }
                    else {
                        if (action.frame != m_fields->m_lastSpawnFrame2P) {
                            shouldSpawnMarker = true;
                            m_fields->m_lastSpawnFrame2P = action.frame;
                        }
                    }

                    if (shouldSpawnMarker) {
                        CCPoint spawnPos = this->m_player1->getPosition();
                        if (action.isPlayer2 && this->m_player2) {
                            spawnPos = this->m_player2->getPosition();
                        }
                        this->spawnFrameWindowMarker(spawnPos, markerText, markerColor);
                    }

                    for (auto& [idStr, preset] : g_labelPresets) {
                        double targetVal = preset.useIF ? static_cast<double>(ifVal) : fw;
                        if (targetVal >= preset.minVal && targetVal <= preset.maxVal) {
                            if (!skipAudio && !preset.audioPath.empty() && preset.showInHud) {
                                SoundManager::playSound(preset.audioPath);
                            }
                            m_fields->m_hudCounts[preset.id]++;
                            if (preset.showInHud) {
                                needsHudUpdate = true;
                                updatedPresets.push_back(preset.id);
                            }
                        }
                    }
                }
                it++;
            }

            m_fields->m_lastFrame = currentFrame;

            if (needsHudUpdate) {
                this->updateHUDCounts();
                if (!skipAudio) {
                    std::sort(updatedPresets.begin(), updatedPresets.end());
                    updatedPresets.erase(std::unique(updatedPresets.begin(), updatedPresets.end()), updatedPresets.end());
                    for (int id : updatedPresets) {
                        this->playHUDCountAnimation(id);
                    }
                }
            }
        }
    }

    void onMyTick(float dt) {
        if (!g_modEnabled) return;
        auto gm = GameManager::sharedState();
        if (gm->getPlayLayer() && !gm->getPlayLayer()->m_isPaused) {
            if (this->m_player1 && !this->m_player1->m_isDead) {
                if (m_fields->m_checkpointRemovalActive) {
                    armCheckpointRestore(m_fields->m_checkpointRemovalRestoreFrame);
                    applyRestoredFrame();
                    m_fields->m_checkpointRemovalActive = false;
                    m_fields->m_checkpointRemovalRestoreFrame = -1;
                }
                else if (m_fields->m_restoreFrameArmed) {
                    applyRestoredFrame();
                }

                int currentFrame = m_fields->m_currentFrame;
                this->updatePrecisionHUD(currentFrame);
                this->updateAndCleanMarkers();
            }
        }
    }

    void spawnFrameWindowMarker(CCPoint pos, const std::string & displayStr, ccColor4F color) {
        auto markerNode = CCNode::create();
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

        circle->drawPolygon(verts, 64, { 0.f, 0.f, 0.f, 0.f }, 4.f, { 0.f, 0.f, 0.f, color.a });
        circle->drawPolygon(verts, 64, { 0.f, 0.f, 0.f, 0.f }, 2.f, { r, g, b, a });
        markerNode->addChild(circle, 0);

        auto label = CCLabelBMFont::create(displayStr.c_str(), "bigFont.fnt");
        label->setAnchorPoint({ 1.f, 0.5f });
        label->setPosition({ -18.f, 0.f });
        label->setScale(0.5f);
        label->setColor({
            static_cast<GLubyte>(color.r * 255),
            static_cast<GLubyte>(color.g * 255),
            static_cast<GLubyte>(color.b * 255)
            });
        label->setOpacity(static_cast<GLubyte>(color.a * 255));
        markerNode->addChild(label, 1);

        if (this->m_uiLayer) {
            this->m_uiLayer->addChild(markerNode);
        }
        else {
            this->addChild(markerNode, 9999);
        }

        if (this->m_objectLayer) {
            markerNode->setPosition(this->m_objectLayer->convertToWorldSpace(pos));
            markerNode->setScale(std::abs(this->m_objectLayer->getScaleY()));
        }
        else {
            markerNode->setPosition(pos);
        }

        m_fields->m_activeMarkers.push_back({ markerNode, pos });
    }
};

class $modify(MyPlayerObject, PlayerObject) {
    void update(float dt) {
        PlayerObject::update(dt);

        auto pl = PlayLayer::get();
        if (pl && !pl->m_isPaused && this == pl->m_player1) {
            static_cast<MyPlayLayer*>(pl)->onPhysicsStep();
        }
    }
};

void triggerHUDRefresh() {
    if (auto pl = PlayLayer::get()) {
        static_cast<MyPlayLayer*>(pl)->recalculateAndRefreshHUD();
    }
}

// 导出当前物理帧号供 CCDirectorHook 调用
int getCurrentPhysicsFrame() {
    if (auto pl = PlayLayer::get()) {
        return static_cast<MyPlayLayer*>(pl)->m_fields->m_currentFrame;
    }
    return 0;
}
