#include "PrecisionSettingsPopup.hpp"
#include "LStarCalcSettingsPopup.hpp"
#include "../Data/State.hpp"
#include "../Math/Calculator.hpp"

using namespace geode::prelude;

bool PrecisionSettingsPopup::init() {
    if (!Popup::init(360.f, 210.f)) return false;
    this->setTitle("Precision Display");

    auto size = m_mainLayer->getContentSize();
    float centerX = size.width / 2;
    auto menu = CCMenu::create();
    menu->setPosition({ 0, 0 });
    m_mainLayer->addChild(menu);

    auto settingsSpr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
    settingsSpr->setScale(0.65f);
    auto settingsBtn = CCMenuItemSpriteExtra::create(
        settingsSpr,
        this,
        menu_selector(PrecisionSettingsPopup::onOpenCalcSettings)
    );
    settingsBtn->setPosition({ size.width - 24.f, size.height - 24.f });
    menu->addChild(settingsBtn);

    auto addToggle = [&](const char* name, bool state, SEL_MenuHandler selector, float x, float y) {
        auto label = CCLabelBMFont::create(name, "bigFont.fnt");
        label->setScale(0.40f);
        label->setAnchorPoint({ 0.f, 0.5f });
        label->setPosition({ x - 70.f, y });
        m_mainLayer->addChild(label);

        auto toggle = CCMenuItemToggler::createWithStandardSprites(this, selector, 0.60f);
        toggle->toggle(state);
        toggle->setPosition({ x + 65.f, y });
        menu->addChild(toggle);
        };

    addToggle("Base L*", g_showBase, menu_selector(PrecisionSettingsPopup::onTBase), centerX - 85.f, 150.f);
    addToggle("Nerve L*", g_showN, menu_selector(PrecisionSettingsPopup::onTN), centerX - 85.f, 120.f);
    addToggle("Fatigue L*", g_showF, menu_selector(PrecisionSettingsPopup::onTF), centerX - 85.f, 90.f);
    addToggle("CPS L*", g_showC, menu_selector(PrecisionSettingsPopup::onTC), centerX - 85.f, 60.f);

    addToggle("Nerve+Fatigue", g_showNF, menu_selector(PrecisionSettingsPopup::onTNF), centerX + 85.f, 150.f);
    addToggle("Nerve+CPS", g_showNC, menu_selector(PrecisionSettingsPopup::onTNC), centerX + 85.f, 120.f);
    addToggle("Fatigue+CPS", g_showFC, menu_selector(PrecisionSettingsPopup::onTFC), centerX + 85.f, 90.f);
    addToggle("Nerve+Fatigue+CPS", g_showNFC, menu_selector(PrecisionSettingsPopup::onTNFC), centerX + 85.f, 60.f);

    m_calcBtnLabel = CCLabelBMFont::create("Start Calculation", "bigFont.fnt");
    m_calcBtnLabel->setScale(0.45f);

    auto btnContainer = CCNode::create();
    btnContainer->setContentSize({ 180.f, 32.f });

    auto bg = CCScale9Sprite::create("GJ_button_01.png");
    bg->setContentSize({ 180.f, 32.f });
    bg->setPosition(btnContainer->getContentSize() / 2);
    btnContainer->addChild(bg);

    m_calcBtnLabel->setPosition(btnContainer->getContentSize() / 2);
    btnContainer->addChild(m_calcBtnLabel);

    auto calcBtn = CCMenuItemSpriteExtra::create(btnContainer, this, menu_selector(PrecisionSettingsPopup::onToggleCalc));
    calcBtn->setPosition({ centerX, 25.f });
    menu->addChild(calcBtn);

    this->scheduleUpdate();
    return true;
}

void PrecisionSettingsPopup::onOpenCalcSettings(CCObject*) {
    if (auto popup = LStarCalcSettingsPopup::create()) {
        popup->show();
    }
}

void PrecisionSettingsPopup::onTBase(CCObject*) { g_showBase = !g_showBase; saveSettings(); g_forcePrecRedraw = true; }
void PrecisionSettingsPopup::onTN(CCObject*) { g_showN = !g_showN; saveSettings(); g_forcePrecRedraw = true; }
void PrecisionSettingsPopup::onTF(CCObject*) { g_showF = !g_showF; saveSettings(); g_forcePrecRedraw = true; }
void PrecisionSettingsPopup::onTC(CCObject*) { g_showC = !g_showC; saveSettings(); g_forcePrecRedraw = true; }
void PrecisionSettingsPopup::onTNF(CCObject*) { g_showNF = !g_showNF; saveSettings(); g_forcePrecRedraw = true; }
void PrecisionSettingsPopup::onTNC(CCObject*) { g_showNC = !g_showNC; saveSettings(); g_forcePrecRedraw = true; }
void PrecisionSettingsPopup::onTFC(CCObject*) { g_showFC = !g_showFC; saveSettings(); g_forcePrecRedraw = true; }
void PrecisionSettingsPopup::onTNFC(CCObject*) { g_showNFC = !g_showNFC; saveSettings(); g_forcePrecRedraw = true; }

void PrecisionSettingsPopup::onToggleCalc(CCObject*) {
    if (g_isCalculating.load()) {
        stopGlobalRecalc();
    }
    else {
        startGlobalRecalc();
    }
}

void PrecisionSettingsPopup::update(float dt) {
    if (g_isCalculating.load()) {
        m_calcBtnLabel->setString(fmt::format("Stop ({:.1f}%)", g_calcProgress.load()).c_str());
        m_calcBtnLabel->setColor({ 255, 100, 100 });
    }
    else {
        m_calcBtnLabel->setString("Start Calculation");
        m_calcBtnLabel->setColor({ 255, 255, 255 });
    }
}

PrecisionSettingsPopup* PrecisionSettingsPopup::create() {
    auto ret = new PrecisionSettingsPopup();
    if (ret && ret->init()) { ret->autorelease(); return ret; }
    delete ret; return nullptr;
}

void PrecisionSettingsPopup::showInstant() {
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