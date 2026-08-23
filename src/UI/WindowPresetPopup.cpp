#include "WindowPresetPopup.hpp"
#include "FrameActionPopup.hpp"
#include "../Data/State.hpp"
#include "../Common.hpp"
#include <Geode/ui/ColorPickPopup.hpp>

using namespace geode::prelude;

bool WindowPresetPopup::init() {
    if (!Popup::init(340.f, 230.f)) return false;
    this->setTitle("Window Preset Settings");

    auto size = m_mainLayer->getContentSize();
    float centerX = size.width / 2.f;

    auto menu = CCMenu::create();
    menu->setPosition({ 0, 0 });
    m_mainLayer->addChild(menu);

    // 0.Swift输入框
    auto swiftLbl = CCLabelBMFont::create("Sw:", "bigFont.fnt");
    swiftLbl->setScale(0.4f);
    swiftLbl->setPosition({ centerX - 120.f, 165.f });
    m_mainLayer->addChild(swiftLbl);

    m_swiftInput = TextInput::create(45.f, "0");
    m_swiftInput->setPosition({ centerX - 85.f, 165.f });
    m_swiftInput->setFilter("0123456789");
    m_swiftInput->setString("0");
    m_mainLayer->addChild(m_swiftInput);

    // 1. Window 输入框与 Load 按钮
    auto winLbl = CCLabelBMFont::create("Win:", "bigFont.fnt");
    winLbl->setScale(0.4f);
    winLbl->setPosition({ centerX - 35.f, 165.f });
    m_mainLayer->addChild(winLbl);

    m_winInput = TextInput::create(55.f, "1");
    m_winInput->setPosition({ centerX + 10.f, 165.f });
    m_winInput->setFilter("0123456789./");
    m_winInput->setString("1");
    m_mainLayer->addChild(m_winInput);

    auto loadBtnSpr = ButtonSprite::create("Load");
    loadBtnSpr->setScale(0.65f);
    auto loadBtn = CCMenuItemSpriteExtra::create(loadBtnSpr, this, menu_selector(WindowPresetPopup::onLoad));
    loadBtn->setPosition({ centerX + 95.f, 165.f });
    menu->addChild(loadBtn);

    // 2. 自定义文本输入框
    m_textInput = TextInput::create(230.f, "Custom Text (Default: Win)");
    m_textInput->setPosition({ centerX, 125.f });
    m_textInput->setFilter("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");
    m_textInput->setCallback([this](std::string const&) { this->autoSave(); });
    m_mainLayer->addChild(m_textInput);
    // 3. 颜色选择按钮
    auto colorLabel = CCLabelBMFont::create("Circle Color:", "bigFont.fnt");
    colorLabel->setScale(0.45f);
    colorLabel->setPosition({ centerX - 40.f, 80.f });
    m_mainLayer->addChild(colorLabel);

    m_colorSprite = CCSprite::createWithSpriteFrameName("GJ_colorBtn_001.png");
    m_colorSprite->setColor({ 255, 255, 255 });
    m_colorSprite->setOpacity(255);

    auto colorWrapper = CCNode::create();
    colorWrapper->setContentSize(m_colorSprite->getContentSize());
    m_colorSprite->setPosition(colorWrapper->getContentSize() / 2);
    colorWrapper->addChild(m_colorSprite);
    colorWrapper->setScale(0.65f);

    auto colorBtn = CCMenuItemSpriteExtra::create(colorWrapper, this, menu_selector(WindowPresetPopup::onColorBtn));
    colorBtn->setPosition({ centerX + 45.f, 80.f });
    menu->addChild(colorBtn);
    // 4. 底部返回与全部重置按钮
    auto switchBtnSpr = ButtonSprite::create("<- Back");
    switchBtnSpr->setScale(0.6f);
    auto switchBtn = CCMenuItemSpriteExtra::create(switchBtnSpr, this, menu_selector(WindowPresetPopup::onSwitchToFrames));
    switchBtn->setPosition({ centerX - 70.f, 30.f });
    menu->addChild(switchBtn);

    auto resetBtnSpr = ButtonSprite::create("Reset All");
    resetBtnSpr->setScale(0.6f);
    auto resetBtn = CCMenuItemSpriteExtra::create(resetBtnSpr, this, menu_selector(WindowPresetPopup::onResetAll));
    resetBtn->setPosition({ centerX + 70.f, 30.f });
    menu->addChild(resetBtn);

    this->onLoad(nullptr);
    return true;
}

void WindowPresetPopup::onLoad(CCObject*) {
    std::string swStr = m_swiftInput ? m_swiftInput->getString() : "0";
    std::string winStr = m_winInput ? m_winInput->getString() : "1";
    if (winStr.empty()) return;

    int swiftVal = 0;
    try { swiftVal = std::stoi(swStr); }
    catch (...) { swiftVal = 0; }
    if (swiftVal < 0) swiftVal = 0;

    double winVal = parseWindowExpr(winStr, 1.0);

    std::string normalizedKey = makeWindowPresetKey(swiftVal, winVal);

    if (g_windowPresets.contains(normalizedKey)) {
        auto& p = g_windowPresets[normalizedKey];
        m_currentColor = p.color;
        if (m_textInput) m_textInput->setString(p.customText);

        if (m_colorSprite) {
            m_colorSprite->setColor({
                static_cast<GLubyte>(p.color.r * 255),
                static_cast<GLubyte>(p.color.g * 255),
                static_cast<GLubyte>(p.color.b * 255)
                });
            m_colorSprite->setOpacity(static_cast<GLubyte>(p.color.a * 255));
        }
    }
    else {
        m_currentColor = { 1.f, 1.f, 1.f, 1.f };
        if (m_textInput) m_textInput->setString("");

        if (m_colorSprite) {
            m_colorSprite->setColor({ 255, 255, 255 });
            m_colorSprite->setOpacity(255);
        }
    }
}

void WindowPresetPopup::onColorBtn(CCObject*) {
    Ref<WindowPresetPopup> safeThis = this;

    auto popup = geode::ColorPickPopup::create({
        static_cast<GLubyte>(m_currentColor.r * 255),
        static_cast<GLubyte>(m_currentColor.g * 255),
        static_cast<GLubyte>(m_currentColor.b * 255),
        static_cast<GLubyte>(m_currentColor.a * 255)
        });

    popup->setCallback([safeThis](cocos2d::ccColor4B color) {
        if (!safeThis || !safeThis->getParent()) return;

        safeThis->m_currentColor = {
            color.r / 255.f,
            color.g / 255.f,
            color.b / 255.f,
            color.a / 255.f
        };

        if (safeThis->m_colorSprite) {
            safeThis->m_colorSprite->setColor({ color.r, color.g, color.b });
            safeThis->m_colorSprite->setOpacity(color.a);
        }

        safeThis->autoSave();
        });

    popup->show();
}

void WindowPresetPopup::autoSave() {
    std::string swStr = m_swiftInput ? m_swiftInput->getString() : "0";
    std::string winStr = m_winInput ? m_winInput->getString() : "1";
    if (winStr.empty()) return;

    int swiftVal = 0;
    try { swiftVal = std::stoi(swStr); }
    catch (...) { swiftVal = 0; }
    if (swiftVal < 0) swiftVal = 0;

    double winVal = parseWindowExpr(winStr, 1.0);

    FrameWindowPreset p;
    p.swift = swiftVal;
    p.window = winVal;
    p.color = m_currentColor;
    p.customText = m_textInput ? m_textInput->getString() : "";

    std::string normalizedKey = makeWindowPresetKey(swiftVal, winVal);
    g_windowPresets[normalizedKey] = p;
    saveSettings();
}

void WindowPresetPopup::onResetAll(CCObject*) {
    Ref<WindowPresetPopup> safeThis = this;

    auto alert = geode::createQuickPopup(
        "Reset Window Presets",
        "Are you sure you want to reset <cr>ALL</c> window presets?",
        "Cancel", "Reset",
        [safeThis](auto, bool btn2) {
            if (btn2) {
                g_windowPresets.clear();
                saveSettings();
                triggerHUDRefresh();

                if (safeThis && safeThis->getParent()) {
                    safeThis->m_currentColor = { 1.f, 1.f, 1.f, 1.f };
                    if (safeThis->m_textInput) safeThis->m_textInput->setString("");
                    if (safeThis->m_colorSprite) {
                        safeThis->m_colorSprite->setColor({ 255, 255, 255 });
                        safeThis->m_colorSprite->setOpacity(255);
                    }
                }

                auto successAlert = FLAlertLayer::create("Success", "All window presets have been reset.", "OK");
                successAlert->show();
                stopAlertAnimation(successAlert);
            }
        }
    );
    stopAlertAnimation(alert);
}

void WindowPresetPopup::onSwitchToFrames(CCObject*) {
    this->removeFromParentAndCleanup(true);
    auto popup = FrameActionPopup::create();
    if (popup) {
        popup->setID("FrameActionPopup"_spr);
        popup->showInstant();
    }
}

WindowPresetPopup* WindowPresetPopup::create() {
    auto ret = new WindowPresetPopup();
    if (ret && ret->init()) { ret->autorelease(); return ret; }
    delete ret; return nullptr;
}

void WindowPresetPopup::showInstant() {
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

void WindowPresetPopup::instantClose() {
    this->removeFromParentAndCleanup(true);
}