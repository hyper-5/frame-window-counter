#include "LStarCalcSettingsPopup.hpp"
#include "../Data/State.hpp"
#include "../Common.hpp"

using namespace geode::prelude;

bool LStarCalcSettingsPopup::init() {
    if (!Popup::init(380.f, 290.f)) return false;
    this->setTitle("L* Calc Settings");

    auto size = m_mainLayer->getContentSize();
    auto menu = CCMenu::create();
    menu->setPosition({ 0, 0 });
    m_mainLayer->addChild(menu);

    auto createRow = [&](const char* labelText, TextInput*& inputRef, float y) {
        auto lbl = CCLabelBMFont::create(labelText, "bigFont.fnt");
        lbl->setScale(0.35f);
        lbl->setAnchorPoint({ 1.0f, 0.5f });
        lbl->setPosition({ size.width * 0.40f - 10.f, y });
        m_mainLayer->addChild(lbl);

        inputRef = TextInput::create(220.f, "0.0", "bigFont.fnt");
        inputRef->setScale(0.65f);
        inputRef->setFilter("0123456789.");
        inputRef->setPosition({ size.width * 0.40f + 80.f, y });
        m_mainLayer->addChild(inputRef);
        };

    float startY = 220.f;
    float stepY = 31.f;

    createRow("TPS:", m_tpsInput, startY);
    createRow("Respawn (s):", m_respawnInput, startY - stepY * 1);
    createRow("Target (s):", m_targetTimeInput, startY - stepY * 2);
    createRow("Nerve (K_T):", m_nerveInput, startY - stepY * 3);
    createRow("Fatigue (K_U):", m_fatigueInput, startY - stepY * 4);
    createRow("CPS (K_C):", m_cpsInput, startY - stepY * 5);

    syncInputsFromState();

    auto resetSpr = ButtonSprite::create("Reset Defaults", "goldFont.fnt", "GJ_button_05.png", 0.8f);
    resetSpr->setScale(0.7f);
    auto resetBtn = CCMenuItemSpriteExtra::create(
        resetSpr,
        this,
        menu_selector(LStarCalcSettingsPopup::onResetDefaults)
    );
    resetBtn->setPosition({ size.width / 2, 25.f });
    menu->addChild(resetBtn);

    return true;
}

void LStarCalcSettingsPopup::syncInputsFromState() {
    if (m_tpsInput) m_tpsInput->setString(fmt::format("{:.2f}", g_macroFps));
    if (m_respawnInput) m_respawnInput->setString(fmt::format("{:.2f}", g_respawnTime));
    if (m_targetTimeInput) m_targetTimeInput->setString(fmt::format("{:.1f}", g_targetTime));
    if (m_nerveInput) m_nerveInput->setString(fmt::format("{:.16f}", g_kT));
    if (m_fatigueInput) m_fatigueInput->setString(fmt::format("{:.16f}", g_kU));
    if (m_cpsInput) m_cpsInput->setString(fmt::format("{:.16f}", g_kC));
}

void LStarCalcSettingsPopup::saveInputs() {
    auto parseVal = [](TextInput* input, double defaultVal) -> double {
        if (!input) return defaultVal;
        try {
            std::string str = input->getString();
            if (str.empty()) return defaultVal;
            return std::stod(str);
        }
        catch (...) {
            return defaultVal;
        }
        };

    double newFps = parseVal(m_tpsInput, DEFAULT_MACRO_FPS);
    double newRespawn = parseVal(m_respawnInput, DEFAULT_RESPAWN_TIME);
    double newTarget = parseVal(m_targetTimeInput, DEFAULT_TARGET_TIME);
    double newKt = parseVal(m_nerveInput, DEFAULT_K_T);
    double newKu = parseVal(m_fatigueInput, DEFAULT_K_U);
    double newKc = parseVal(m_cpsInput, DEFAULT_K_C);

    if (newFps != g_macroFps || newRespawn != g_respawnTime || newTarget != g_targetTime ||
        newKt != g_kT || newKu != g_kU || newKc != g_kC) {
        g_isLStarDirty = true;
    }

    g_macroFps = newFps;
    g_respawnTime = newRespawn;
    g_targetTime = newTarget;
    g_kT = newKt;
    g_kU = newKu;
    g_kC = newKc;

    saveSettings();
}

void LStarCalcSettingsPopup::onResetDefaults(CCObject*) {
    resetCalcSettingsToDefault();
    syncInputsFromState();
    FLAlertLayer::create("Settings Reset", "All calculation settings restored to default.", "OK")->show();
}

void LStarCalcSettingsPopup::onClose(CCObject* sender) {
    saveInputs();
    Popup::onClose(sender);
}

LStarCalcSettingsPopup* LStarCalcSettingsPopup::create() {
    auto ret = new LStarCalcSettingsPopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}