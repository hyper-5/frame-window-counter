#include "WindowPresetPopup.hpp"
#include "FrameActionPopup.hpp"
#include "../Data/State.hpp"
#include "../Common.hpp"
#include <Geode/ui/ColorPickPopup.hpp>

using namespace geode::prelude;

bool WindowPresetPopup::init() {
    if (!Popup::init(300.f, 200.f)) return false;
    this->setTitle("Window Color Settings");

    auto size = m_mainLayer->getContentSize();
    float centerX = size.width / 2;

    auto menu = CCMenu::create();
    menu->setPosition({ 0, 0 });
    m_mainLayer->addChild(menu);

    m_winInput = TextInput::create(140.f, "Window (e.g. 4)");
    m_winInput->setPosition({ centerX - 50.f, 130.f });
    m_winInput->setFilter("0123456789");
    m_mainLayer->addChild(m_winInput);

    auto loadBtn = CCMenuItemSpriteExtra::create(ButtonSprite::create("Load"), this, menu_selector(WindowPresetPopup::onLoad));
    loadBtn->setPosition({ centerX + 60.f, 130.f });
    menu->addChild(loadBtn);

    auto colorLabel = CCLabelBMFont::create("Circle Color:", "bigFont.fnt");
    colorLabel->setScale(0.5f);
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
    colorBtn->setPosition({ centerX + 40.f, 80.f });
    menu->addChild(colorBtn);

    auto switchBtnSpr = ButtonSprite::create("<- Back to Frames");
    switchBtnSpr->setScale(0.7f);
    auto switchBtn = CCMenuItemSpriteExtra::create(switchBtnSpr, this, menu_selector(WindowPresetPopup::onSwitchToFrames));
    switchBtn->setPosition({ centerX, 30.f });
    menu->addChild(switchBtn);

    return true;
}

void WindowPresetPopup::onLoad(CCObject*) {
    std::string winStr = m_winInput->getString();
    if (g_windowPresets.contains(winStr)) {
        auto& p = g_windowPresets[winStr];
        m_currentColor = p.color;
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
    std::string winStr = m_winInput->getString();
    if (winStr.empty()) return;

    FrameWindowPreset p;
    try { p.window = std::stoi(winStr); }
    catch (...) { p.window = 1; }
    p.color = m_currentColor;

    g_windowPresets[winStr] = p;
    saveSettings();
}

void WindowPresetPopup::onSwitchToFrames(CCObject*) {
    this->instantClose();
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