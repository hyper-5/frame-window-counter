#include "FrameActionPopup.hpp"
#include "AddFramePopup.hpp"
#include "WindowPresetPopup.hpp"
#include "LabelPresetPopup.hpp"
#include "PrecisionSettingsPopup.hpp"
#include "../Data/State.hpp"
#include "../IO/FileIO.hpp"
#include "../Common.hpp"
#include <algorithm>
#include <cmath>

using namespace geode::prelude;

bool FrameActionPopup::init() {
    if (!Popup::init(430.f, 280.f)) return false;
    this->setTitle("Frame Window Editor");

    auto size = m_mainLayer->getContentSize();
    float centerX = size.width / 2;

    auto topMenu = CCMenu::create();
    topMenu->setPosition({ 0, 0 });
    m_mainLayer->addChild(topMenu);

    auto checkOffSpr = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
    auto checkOnSpr = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
    checkOffSpr->setScale(0.5f);
    checkOnSpr->setScale(0.5f);

    auto enableToggle = CCMenuItemToggler::create(checkOffSpr, checkOnSpr, this, menu_selector(FrameActionPopup::onToggleMod));
    enableToggle->toggle(g_modEnabled);
    enableToggle->setPosition({ 35.f, size.height - 28.f });
    topMenu->addChild(enableToggle);

    auto enableLabel = CCLabelBMFont::create("Enabled", "bigFont.fnt");
    enableLabel->setScale(0.35f);
    enableLabel->setAnchorPoint({ 0.f, 0.5f });
    enableLabel->setPosition({ 50.f, size.height - 28.f });
    m_mainLayer->addChild(enableLabel);

    auto precSpr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
    precSpr->setScale(0.65f);
    auto precBtn = CCMenuItemSpriteExtra::create(precSpr, this, menu_selector(FrameActionPopup::onOpenPrecision));
    precBtn->setPosition({ size.width - 25.f, size.height - 25.f });
    topMenu->addChild(precBtn);

    auto addSpr = CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png");
    addSpr->setScale(0.6f);
    auto addBtn = CCMenuItemSpriteExtra::create(addSpr, this, menu_selector(FrameActionPopup::onAddFramePrompt));
    addBtn->setPosition({ size.width - 60.f, size.height - 25.f });
    topMenu->addChild(addBtn);

    auto clearAllSpr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
    clearAllSpr->setScale(0.68f);
    auto clearAllBtn = CCMenuItemSpriteExtra::create(clearAllSpr, this, menu_selector(FrameActionPopup::onClearAll));
    clearAllBtn->setPosition({ size.width - 95.f, size.height - 25.f });
    topMenu->addChild(clearAllBtn);

    float topY = 225.f;

    auto allOnBtnSpr = ButtonSprite::create("Activate All");
    allOnBtnSpr->setScale(0.5f);
    auto allOnBtn = CCMenuItemSpriteExtra::create(allOnBtnSpr, this, menu_selector(FrameActionPopup::onSelectAll));
    allOnBtn->setPosition({ 66.f, topY });
    topMenu->addChild(allOnBtn);

    auto allOffBtnSpr = ButtonSprite::create("Inactivate All");
    allOffBtnSpr->setScale(0.5f);
    auto allOffBtn = CCMenuItemSpriteExtra::create(allOffBtnSpr, this, menu_selector(FrameActionPopup::onDeselectAll));
    allOffBtn->setPosition({ size.width - 71.f, topY });
    topMenu->addChild(allOffBtn);

    auto prevSpr = CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png");
    prevSpr->setScale(0.4f);
    m_prevBtn = CCMenuItemSpriteExtra::create(prevSpr, this, menu_selector(FrameActionPopup::onPrevPage));
    m_prevBtn->setPosition({ centerX - 70.f, topY });
    topMenu->addChild(m_prevBtn);

    m_pageInput = TextInput::create(40.f, "1");
    m_pageInput->setPosition({ centerX - 30.f, topY });
    m_pageInput->setFilter("0123456789");
    m_pageInput->setScale(0.65f);
    m_pageInput->setCallback([this](std::string const& text) {
        if (text.empty()) return;
        int targetPage = 0;
        try { targetPage = std::stoi(text) - 1; }
        catch (...) { targetPage = 999999; }
        int totalPages = std::max(1, (int)(g_frameActions.size() + m_itemsPerPage - 1) / m_itemsPerPage);
        if (targetPage < 0) targetPage = 0;
        if (targetPage >= totalPages) targetPage = totalPages - 1;

        if (m_currentPage != targetPage) {
            m_currentPage = targetPage;
            refreshList();
        }
        });
    m_mainLayer->addChild(m_pageInput);

    m_totalPagesLabel = CCLabelBMFont::create("/ 1", "bigFont.fnt");
    m_totalPagesLabel->setPosition({ centerX + 10.f, topY });
    m_totalPagesLabel->setScale(0.4f);
    m_mainLayer->addChild(m_totalPagesLabel);

    auto nextSpr = CCSprite::createWithSpriteFrameName("edit_rightBtn_001.png");
    nextSpr->setScale(0.4f);
    m_nextBtn = CCMenuItemSpriteExtra::create(nextSpr, this, menu_selector(FrameActionPopup::onNextPage));
    m_nextBtn->setPosition({ centerX + 60.f, topY });
    topMenu->addChild(m_nextBtn);

    m_scrollLayer = ScrollLayer::create({ 400.f, 165.f });
    m_scrollLayer->setPosition({ 15.f, 45.f });
    m_mainLayer->addChild(m_scrollLayer);

    auto contentNode = CCMenu::create();
    contentNode->setPosition({ 0, 0 });
    contentNode->setID("content-node"_spr);
    m_scrollLayer->m_contentLayer->addChild(contentNode);

    for (int i = 0; i < m_itemsPerPage; i++) {
        auto cell = createCellTemplate(i);
        contentNode->addChild(cell);
    }

    refreshList(true);

    auto bottomMenu = CCMenu::create();
    bottomMenu->setPosition({ 0, 0 });

    float bottomY = 25.f;
    auto backBtnSpr = ButtonSprite::create("Labels");
    backBtnSpr->setScale(0.55f);
    auto backBtn = CCMenuItemSpriteExtra::create(backBtnSpr, this, menu_selector(FrameActionPopup::onSwitchToLabels));
    backBtn->setPosition({ centerX - 135.f, bottomY });
    bottomMenu->addChild(backBtn);

    auto winBtnSpr = ButtonSprite::create("Windows");
    winBtnSpr->setScale(0.55f);
    auto winBtn = CCMenuItemSpriteExtra::create(winBtnSpr, this, menu_selector(FrameActionPopup::onSwitchToWindows));
    winBtn->setPosition({ centerX - 45.f, bottomY });
    bottomMenu->addChild(winBtn);

    auto importBtnSpr = ButtonSprite::create("Import");
    importBtnSpr->setScale(0.55f);
    auto importBtn = CCMenuItemSpriteExtra::create(importBtnSpr, this, menu_selector(FrameActionPopup::onImportGDR));
    importBtn->setPosition({ centerX + 45.f, bottomY });
    bottomMenu->addChild(importBtn);

    auto exportBtnSpr = ButtonSprite::create("Export");
    exportBtnSpr->setScale(0.55f);
    auto exportBtn = CCMenuItemSpriteExtra::create(exportBtnSpr, this, menu_selector(FrameActionPopup::onExportFWC));
    exportBtn->setPosition({ centerX + 135.f, bottomY });
    bottomMenu->addChild(exportBtn);

    m_mainLayer->addChild(bottomMenu);

    m_computingLabel = CCLabelBMFont::create("Calculating L*: 0.0%", "bigFont.fnt");
    m_computingLabel->setScale(0.35f);
    m_computingLabel->setAnchorPoint({ 0.5f, 0.5f });
    m_computingLabel->setPosition({ centerX, size.height - 35.f });
    m_computingLabel->setColor({ 255, 200, 50 });
    m_mainLayer->addChild(m_computingLabel);

    m_finalLStarLabel = CCLabelBMFont::create("Base L*: 0.00", "bigFont.fnt");
    m_finalLStarLabel->setColor({ 255, 255, 255 });
    m_finalLStarLabel->setScale(0.35f);

    auto btnContainer = CCNode::create();
    btnContainer->setContentSize({ 200.f, 25.f });
    m_finalLStarLabel->setPosition(btnContainer->getContentSize() / 2);
    btnContainer->addChild(m_finalLStarLabel);

    m_finalLStarBtn = CCMenuItemSpriteExtra::create(btnContainer, this, menu_selector(FrameActionPopup::onCycleLStar));
    m_finalLStarBtn->setPosition({ centerX, size.height - 35.f });
    topMenu->addChild(m_finalLStarBtn);

    m_wasCalculating = true;
    this->scheduleUpdate();

    return true;
}

void FrameActionPopup::updateFinalLStarText() {
    if (!m_finalLStarLabel || g_validActions.empty()) return;

    std::string prefix = "";
    double val = 0.0;
    switch (g_finalLStarDisplayMode) {
    case 0: prefix = "Base L*: ";          if (!g_vBase.empty()) val = g_vBase.back(); break;
    case 1: prefix = "Nerve L*: ";         if (!g_vN.empty())    val = g_vN.back(); break;
    case 2: prefix = "Fatigue L*: ";       if (!g_vF.empty())    val = g_vF.back(); break;
    case 3: prefix = "CPS L*: ";           if (!g_vC.empty())    val = g_vC.back(); break;
    case 4: prefix = "Nerve+Fatigue L*: "; if (!g_vNF.empty())   val = g_vNF.back(); break;
    case 5: prefix = "Nerve+CPS L*: ";     if (!g_vNC.empty())   val = g_vNC.back(); break;
    case 6: prefix = "Fatigue+CPS L*: ";   if (!g_vFC.empty())   val = g_vFC.back(); break;
    case 7: prefix = "Nerve+Fatigue+CPS L*: "; if (!g_vNFC.empty()) val = g_vNFC.back(); break;
    }

    m_finalLStarLabel->setString(fmt::format("{}{:.2f}", prefix, val).c_str());
    m_finalLStarLabel->setColor(g_isLStarDirty.load() ? ccColor3B{ 255, 100, 100 } : ccColor3B{ 255, 255, 255 });

    if (auto container = m_finalLStarLabel->getParent()) {
        m_finalLStarLabel->setPosition(container->getContentSize() / 2);
    }
}

void FrameActionPopup::onCycleLStar(CCObject*) {
    g_finalLStarDisplayMode = (g_finalLStarDisplayMode + 1) % 8;
    updateFinalLStarText();
}

void FrameActionPopup::update(float dt) {
    bool isCalc = g_isCalculating.load();

    if (isCalc) {
        if (!m_computingLabel->isVisible()) m_computingLabel->setVisible(true);
        if (m_finalLStarBtn->isVisible()) m_finalLStarBtn->setVisible(false);

        m_computingLabel->setString(fmt::format("Calculating L*: {:.1f}%", g_calcProgress.load()).c_str());
        m_wasCalculating = true;
    }
    else {
        if (m_computingLabel->isVisible()) m_computingLabel->setVisible(false);

        if (g_validActions.empty()) {
            if (m_finalLStarBtn->isVisible()) m_finalLStarBtn->setVisible(false);
        }
        else {
            if (!m_finalLStarBtn->isVisible()) m_finalLStarBtn->setVisible(true);

            if (m_wasCalculating || m_wasDirty != g_isLStarDirty.load()) {
                updateFinalLStarText();
                m_wasCalculating = false;
                m_wasDirty = g_isLStarDirty.load();
            }
        }
    }
}

void FrameActionPopup::onAddFramePrompt(CCObject*) {
    if (auto popup = AddFramePopup::create(this)) {
        popup->showInstant();
    }
}

void FrameActionPopup::onClearAll(CCObject*) {
    Ref<FrameActionPopup> safeThis = this;

    auto alert = geode::createQuickPopup(
        "Clear All Frames",
        "Are you sure you want to delete <cr>ALL</c> frames?",
        "Cancel", "Delete",
        [safeThis](auto, bool btn2) {
            if (btn2) {
                g_frameActions.clear();
                saveFrames();

                if (safeThis && safeThis->getParent()) {
                    safeThis->m_currentPage = 0;
                    safeThis->refreshList(true);
                }

                auto successAlert = FLAlertLayer::create("Success", "All frames have been deleted.", "OK");
                successAlert->show();
                stopAlertAnimation(successAlert);
            }
        }
    );
    stopAlertAnimation(alert);
}

void FrameActionPopup::onDeleteFrame(CCObject* sender) {
    auto btn = typeinfo_cast<CCMenuItemSpriteExtra*>(sender);
    if (!btn) return;
    auto strObj = static_cast<cocos2d::CCString*>(btn->getUserObject());
    if (!strObj) return;
    std::string keyStr = strObj->getCString();

    if (g_frameActions.contains(keyStr)) {
        float savedScrollY = 0.f;
        if (m_scrollLayer && m_scrollLayer->m_contentLayer) {
            savedScrollY = m_scrollLayer->m_contentLayer->getPositionY();
        }

        g_frameActions.erase(keyStr);
        saveFrames();
        this->refreshList(true);

        if (m_scrollLayer && m_scrollLayer->m_contentLayer) {
            m_scrollLayer->m_contentLayer->setPositionY(savedScrollY);
        }
    }
}

void FrameActionPopup::jumpToFrame(int targetFrame) {
    this->refreshList(true);
    int targetIndex = -1;
    for (size_t i = 0; i < m_sortedKeys.size(); i++) {
        if (g_frameActions[m_sortedKeys[i]].frame >= targetFrame) {
            targetIndex = i;
            break;
        }
    }
    if (targetIndex == -1 && !m_sortedKeys.empty()) {
        targetIndex = m_sortedKeys.size() - 1;
    }

    if (targetIndex != -1) {
        int targetPage = targetIndex / m_itemsPerPage;
        if (m_currentPage != targetPage) {
            m_currentPage = targetPage;
            this->refreshList();
        }
        m_lastHighlightedFrame = g_frameActions[m_sortedKeys[targetIndex]].frame;
        this->highlightCell(m_lastHighlightedFrame);
        this->scrollToCell(targetIndex % m_itemsPerPage);
    }
}

void FrameActionPopup::onOpenPrecision(CCObject*) {
    if (auto popup = PrecisionSettingsPopup::create()) {
        popup->showInstant();
    }
}

void FrameActionPopup::onToggleMod(CCObject* sender) {
    g_modEnabled = !g_modEnabled;
    saveSettings();
    triggerHUDRefresh();

    if (!g_modEnabled) {
        if (auto pl = GameManager::sharedState()->getPlayLayer()) {
            if (auto hud = pl->getChildByID("frame-window-hud"_spr)) hud->removeFromParent();
            if (pl->m_uiLayer) {
                if (auto hud = pl->m_uiLayer->getChildByID("frame-window-hud"_spr)) hud->removeFromParent();
            }
            if (auto prec = pl->getChildByID("frame-window-prec"_spr)) prec->removeFromParent();
            if (pl->m_uiLayer) {
                if (auto prec = pl->m_uiLayer->getChildByID("frame-window-prec"_spr)) prec->removeFromParent();
            }
            if (pl->m_objectLayer) {
                auto children = pl->m_objectLayer->getChildren();
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
    }
    else {
        g_forcePrecRedraw = true;
    }
}

void FrameActionPopup::onSelectAll(CCObject*) {
    for (auto& [k, v] : g_frameActions) v.shouldDraw = true;
    saveFrames();
    this->refreshList();
}

void FrameActionPopup::onDeselectAll(CCObject*) {
    for (auto& [k, v] : g_frameActions) v.shouldDraw = false;
    saveFrames();
    this->refreshList();
}

void FrameActionPopup::onPrevPage(CCObject*) {
    if (m_currentPage > 0) {
        m_currentPage--;
        refreshList();
    }
}

void FrameActionPopup::onNextPage(CCObject*) {
    m_currentPage++;
    refreshList();
}

void FrameActionPopup::refreshList(bool rebuildKeys) {
    if (rebuildKeys) {
        std::vector<std::pair<std::string, int>> temp;
        temp.reserve(g_frameActions.size());
        for (auto& [k, v] : g_frameActions) {
            temp.push_back({ k, v.frame });
        }
        std::stable_sort(temp.begin(), temp.end(), [](const auto& a, const auto& b) {
            return a.second < b.second;
            });
        m_sortedKeys.clear();
        m_sortedKeys.reserve(temp.size());
        for (auto& p : temp) m_sortedKeys.push_back(p.first);
    }

    int totalPages = std::max(1, (int)(m_sortedKeys.size() + m_itemsPerPage - 1) / m_itemsPerPage);
    if (m_currentPage >= totalPages) m_currentPage = totalPages - 1;
    if (m_currentPage < 0) m_currentPage = 0;

    if (m_pageInput) {
        std::string newPageStr = std::to_string(m_currentPage + 1);
        if (m_pageInput->getString() != newPageStr) m_pageInput->setString(newPageStr);
    }
    if (m_totalPagesLabel) m_totalPagesLabel->setString(fmt::format("/ {}", totalPages).c_str());
    if (m_prevBtn) m_prevBtn->setVisible(m_currentPage > 0);
    if (m_nextBtn) m_nextBtn->setVisible(m_currentPage < totalPages - 1);

    if (!m_scrollLayer || !m_scrollLayer->m_contentLayer) return;
    auto contentNode = m_scrollLayer->m_contentLayer->getChildByID("content-node"_spr);
    if (!contentNode) return;

    int startIdx = m_currentPage * m_itemsPerPage;
    int endIdx = std::min((int)m_sortedKeys.size(), startIdx + m_itemsPerPage);
    int displayCount = endIdx - startIdx;

    float rowHeight = 42.f;
    float contentHeight = std::max(180.f, displayCount * rowHeight);
    contentNode->setContentSize({ 400.f, contentHeight });
    m_scrollLayer->m_contentLayer->setContentSize({ 400.f, contentHeight });

    float currentY = contentHeight - (rowHeight / 2);

    for (int i = 0; i < m_itemsPerPage; i++) {
        auto cell = contentNode->getChildByID(fmt::format("cell_{}", i));
        if (!cell) continue;

        int dataIdx = startIdx + i;
        if (dataIdx < endIdx) {
            cell->setVisible(true);
            cell->setPosition({ 200.f, currentY });
            currentY -= rowHeight;

            const std::string& actionKey = m_sortedKeys[dataIdx];
            const FrameAction& action = g_frameActions[actionKey];
            cell->setTag(action.frame);

            if (auto bg = static_cast<CCScale9Sprite*>(cell->getChildByID("cell-bg"_spr))) {
                if (action.frame == m_lastHighlightedFrame) {
                    bg->setColor({ 255, 100, 100 });
                    bg->setOpacity(200);
                }
                else {
                    bg->setColor({ 255, 255, 255 });
                    bg->setOpacity(100);
                }
            }

            if (auto frameText = static_cast<CCLabelBMFont*>(cell->getChildByID("frame-text"_spr))) {
                frameText->setString(fmt::format("Frame: {}", action.frame).c_str());
            }

            if (auto menu = cell->getChildByType<CCMenu>(0)) {
                if (auto pToggleBtn = static_cast<CCMenuItemSpriteExtra*>(menu->getChildByID("p-toggle-btn"_spr))) {
                    pToggleBtn->setUserObject(cocos2d::CCString::create(actionKey));
                    if (auto spr = typeinfo_cast<ButtonSprite*>(pToggleBtn->getNormalImage())) {
                        spr->setString(action.isPlayer2 ? "2P" : "1P");
                    }
                }
                if (auto drawToggle = static_cast<CCMenuItemToggler*>(menu->getChildByID("draw-toggle"_spr))) {
                    drawToggle->setUserObject(cocos2d::CCString::create(actionKey));
                    drawToggle->toggle(action.shouldDraw);
                }
                if (auto delBtn = static_cast<CCMenuItemSpriteExtra*>(menu->getChildByID("del-btn"_spr))) {
                    delBtn->setUserObject(cocos2d::CCString::create(actionKey));
                }
            }

            if (auto winInput = static_cast<TextInput*>(cell->getChildByID("win-input"_spr))) {
                winInput->setUserObject(cocos2d::CCString::create(actionKey));
                std::string winStr = formatWindowVal(action.frameWindow);
                if (winInput->getString() != winStr) winInput->setString(winStr);
            }

            // 同步 Swift 输入框值
            if (auto swiftInput = static_cast<TextInput*>(cell->getChildByID("swift-input"_spr))) {
                swiftInput->setUserObject(cocos2d::CCString::create(actionKey));
                std::string swStr = std::to_string(action.swift);
                if (swiftInput->getString() != swStr) swiftInput->setString(swStr);
            }
        }
        else {
            cell->setVisible(false);
        }
    }
    m_scrollLayer->moveToTop();
}

CCNode* FrameActionPopup::createCellTemplate(int index) {
    auto cell = CCNode::create();
    cell->setContentSize({ 390.f, 40.f });
    cell->setAnchorPoint({ 0.5f, 0.5f });
    cell->setID(fmt::format("cell_{}", index));

    auto bg = CCScale9Sprite::create("square02_small.png");
    bg->setContentSize({ 390.f, 38.f });
    bg->setOpacity(100);
    bg->setPosition({ 195.f, 20.f });
    bg->setID("cell-bg"_spr);
    cell->addChild(bg);

    auto menu = CCMenu::create();
    menu->setPosition({ 0.f, 0.f });
    cell->addChild(menu);

    auto frameText = CCLabelBMFont::create("Frame: 0", "chatFont.fnt");
    frameText->setAnchorPoint({ 0.f, 0.5f });
    frameText->setPosition({ 8.f, 20.f });
    frameText->setScale(0.5f);
    frameText->setID("frame-text"_spr);
    cell->addChild(frameText);

    auto pToggleSpr = ButtonSprite::create("1P");
    pToggleSpr->setScale(0.42f);
    auto pToggleBtn = CCMenuItemSpriteExtra::create(pToggleSpr, this, menu_selector(FrameActionPopup::onTogglePlayer));
    pToggleBtn->setPosition({ 76.f, 20.f });
    pToggleBtn->setID("p-toggle-btn"_spr);
    menu->addChild(pToggleBtn);

    auto drawToggle = CCMenuItemToggler::create(
        CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png"),
        CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png"),
        this, menu_selector(FrameActionPopup::onToggleDraw)
    );
    drawToggle->setPosition({ 110.f, 20.f });
    drawToggle->setScale(0.65f);
    drawToggle->setID("draw-toggle"_spr);
    menu->addChild(drawToggle);

    auto typeText = CCLabelBMFont::create("Frame Window:", "chatFont.fnt");
    typeText->setAnchorPoint({ 0.f, 0.5f });
    typeText->setPosition({ 145.f, 20.f });
    typeText->setScale(0.45f);
    cell->addChild(typeText);

    auto winInput = TextInput::create(42.f, "1");
    winInput->setFilter("0123456789./");
    winInput->setPosition({ 215.f, 20.f });
    winInput->setScale(0.85f);
    winInput->setID("win-input"_spr);
    winInput->setCallback([winInput](std::string const& text) {
        auto strObj = static_cast<cocos2d::CCString*>(winInput->getUserObject());
        if (!strObj) return;
        std::string actionKey = strObj->getCString();

        if (g_frameActions.contains(actionKey)) {
            double fwVal = parseWindowExpr(text, 1.0);
            if (std::abs(g_frameActions[actionKey].frameWindow - fwVal) > 1e-6) {
                g_frameActions[actionKey].frameWindow = fwVal;
                saveFrames();
            }
        }
        });
    cell->addChild(winInput);

    auto swiftText = CCLabelBMFont::create("Swift:", "chatFont.fnt");
    swiftText->setAnchorPoint({ 0.f, 0.5f });
    swiftText->setPosition({ 260.f, 20.f });
    swiftText->setScale(0.45f);
    cell->addChild(swiftText);

    auto swiftInput = TextInput::create(38.f, "0");
    swiftInput->setFilter("0123456789");
    swiftInput->setPosition({ 305.f, 20.f });
    swiftInput->setScale(0.85f);
    swiftInput->setID("swift-input"_spr);
    swiftInput->setCallback([swiftInput](std::string const& text) {
        auto strObj = static_cast<cocos2d::CCString*>(swiftInput->getUserObject());
        if (!strObj) return;
        std::string actionKey = strObj->getCString();

        if (g_frameActions.contains(actionKey)) {
            int swVal = 0;
            if (!text.empty()) {
                try { swVal = std::stoi(text); }
                catch (...) { swVal = 0; }
            }
            if (swVal < 0) swVal = 0;
            if (g_frameActions[actionKey].swift != swVal) {
                g_frameActions[actionKey].swift = swVal;
                updateTickCache();
                triggerHUDRefresh();
            }
        }
        });
    cell->addChild(swiftInput);

    auto delRowSpr = CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png");
    delRowSpr->setScale(0.55f);
    auto delRowBtn = CCMenuItemSpriteExtra::create(delRowSpr, this, menu_selector(FrameActionPopup::onDeleteFrame));
    delRowBtn->setPosition({ 360.f, 20.f });
    delRowBtn->setID("del-btn"_spr);
    menu->addChild(delRowBtn);

    return cell;
}

void FrameActionPopup::onToggleDraw(CCObject* sender) {
    auto toggle = typeinfo_cast<CCMenuItemToggler*>(sender);
    if (!toggle) return;
    auto strObj = static_cast<cocos2d::CCString*>(toggle->getUserObject());
    if (!strObj) return;
    std::string keyStr = strObj->getCString();

    if (g_frameActions.contains(keyStr)) {
        g_frameActions[keyStr].shouldDraw = !toggle->isToggled();
        saveFrames();
    }
}

void FrameActionPopup::onTogglePlayer(CCObject* sender) {
    auto btn = typeinfo_cast<CCMenuItemSpriteExtra*>(sender);
    if (!btn) return;
    auto strObj = static_cast<cocos2d::CCString*>(btn->getUserObject());
    if (!strObj) return;
    std::string keyStr = strObj->getCString();

    if (!g_frameActions.contains(keyStr)) return;

    auto& action = g_frameActions[keyStr];
    action.isPlayer2 = !action.isPlayer2;
    bool isP2 = action.isPlayer2;

    auto it = std::lower_bound(g_tickActionsCache.begin(), g_tickActionsCache.end(), action.frame,
        [](const FrameAction& a, int frame) { return a.frame < frame; });
    while (it != g_tickActionsCache.end() && it->frame == action.frame) {
        if (it->isPlayer2 != isP2 && std::abs(it->frameWindow - action.frameWindow) < 1e-6 && it->swift == action.swift) {
            it->isPlayer2 = isP2;
            break;
        }
        ++it;
    }

    if (auto spr = typeinfo_cast<ButtonSprite*>(btn->getNormalImage())) {
        spr->setString(isP2 ? "2P" : "1P");
    }
}

void FrameActionPopup::onSwitchToLabels(CCObject*) {
    this->instantClose();
    auto popup = LabelPresetPopup::create();
    if (popup) {
        popup->setID("LabelPresetPopup"_spr);
        popup->showInstant();
    }
}

void FrameActionPopup::onSwitchToWindows(CCObject*) {
    this->instantClose();
    auto popup = WindowPresetPopup::create();
    if (popup) {
        popup->setID("WindowPresetPopup"_spr);
        popup->showInstant();
    }
}

void FrameActionPopup::onExportFWC(CCObject*) {
    FileIO::exportFWC();
}

void FrameActionPopup::onImportGDR(CCObject*) {
    Ref<FrameActionPopup> safeThis = this;
    FileIO::importReplay([safeThis]() {
        if (safeThis->getParent()) {
            safeThis->m_currentPage = 0;
            safeThis->refreshList(true);
        }
        });
}

FrameActionPopup* FrameActionPopup::create() {
    auto ret = new FrameActionPopup();
    if (ret && ret->init()) { ret->autorelease(); return ret; }
    delete ret; return nullptr;
}

void FrameActionPopup::showInstant() {
    this->show();
    if (this->m_mainLayer) {
        this->m_mainLayer->stopAllActions();
        this->m_mainLayer->setScale(1.0f);
    }
    if (this->m_bgSprite) {
        this->m_bgSprite->stopAllActions();
        this->m_bgSprite->setOpacity(150);
    }
}

void FrameActionPopup::instantClose() {
    this->removeFromParentAndCleanup(true);
}

void FrameActionPopup::doTrackingTick(int currentFrame) {
    if (m_sortedKeys.empty()) return;

    int targetIndex = -1;
    auto it = std::lower_bound(m_sortedKeys.begin(), m_sortedKeys.end(), currentFrame,
        [](const std::string& key, int frame) {
            return g_frameActions.at(key).frame < frame;
        });

    if (it != m_sortedKeys.end()) {
        targetIndex = std::distance(m_sortedKeys.begin(), it);
    }

    if (targetIndex != -1) {
        int targetFrame = g_frameActions[m_sortedKeys[targetIndex]].frame;
        if (targetFrame != m_lastHighlightedFrame) {
            m_lastHighlightedFrame = targetFrame;
            int targetPage = targetIndex / m_itemsPerPage;

            if (m_currentPage != targetPage) {
                m_currentPage = targetPage;
                this->refreshList();
                this->scrollToCell(targetIndex % m_itemsPerPage);
            }
            else {
                this->highlightCell(targetFrame);
                this->scrollToCell(targetIndex % m_itemsPerPage);
            }
        }
    }
    else {
        if (m_lastHighlightedFrame != -1) {
            m_lastHighlightedFrame = -1;
            this->highlightCell(-1);
        }
    }
}

void FrameActionPopup::highlightCell(int targetFrame) {
    if (!m_scrollLayer || !m_scrollLayer->m_contentLayer) return;
    auto contentNode = m_scrollLayer->m_contentLayer->getChildByID("content-node"_spr);
    if (!contentNode) return;

    auto children = contentNode->getChildren();
    if (!children) return;

    for (int i = 0; i < children->count(); i++) {
        auto cell = static_cast<CCNode*>(children->objectAtIndex(i));
        auto bg = static_cast<CCScale9Sprite*>(cell->getChildByID("cell-bg"_spr));
        if (bg) {
            if (cell->getTag() == targetFrame) {
                bg->setColor({ 255, 100, 100 });
                bg->setOpacity(200);
            }
            else {
                bg->setColor({ 255, 255, 255 });
                bg->setOpacity(100);
            }
        }
    }
}

void FrameActionPopup::scrollToCell(int indexInPage) {
    if (!m_scrollLayer || !m_scrollLayer->m_contentLayer) return;

    float rowHeight = 42.f;
    float viewHeight = 180.f;
    float contentHeight = m_scrollLayer->m_contentLayer->getContentSize().height;

    float targetY = contentHeight - (indexInPage * rowHeight) - rowHeight / 2.f;
    float scrollY = viewHeight / 2.f - targetY;

    float minY = viewHeight - contentHeight;
    float maxY = 0.f;
    if (minY > 0) minY = 0;

    if (scrollY < minY) scrollY = minY;
    if (scrollY > maxY) scrollY = maxY;

    m_scrollLayer->m_contentLayer->setPositionY(scrollY);
}