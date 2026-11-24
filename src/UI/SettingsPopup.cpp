#include "SettingsPopup.hpp"
#include "../Data/State.hpp"
#include "../Math/Calculator.hpp"
#include "../Common.hpp"
#if defined(GEODE_IS_ANDROID) || defined(GEODE_IS_IOS)
#include "MobileAudioPickerPopup.hpp"
#endif
#include <Geode/ui/ColorPickPopup.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/async.hpp>
#include <algorithm>
#include <cmath>
#include <optional>
#include <set>

using namespace geode::prelude;

SettingsTab SettingsPopup::s_lastTab = SettingsTab::Display;

namespace {
    constexpr float kCircleMin = 0.2f, kCircleMax = 3.0f;
    constexpr float kMarkerTextMin = 0.1f, kMarkerTextMax = 2.0f;
    constexpr float kHudTextMin = 0.1f, kHudTextMax = 1.5f;
    constexpr float kHudSpaceMin = 6.0f, kHudSpaceMax = 50.0f;
    constexpr float kPrecTextMin = 0.1f, kPrecTextMax = 1.2f;
    constexpr float kPrecSpaceMin = 4.0f, kPrecSpaceMax = 40.0f;

    float toNorm(float val, float minV, float maxV) {
        return std::clamp((val - minV) / (maxV - minV), 0.f, 1.f);
    }

    float fromNorm(float norm, float minV, float maxV) {
        return minV + std::clamp(norm, 0.f, 1.f) * (maxV - minV);
    }

    constexpr const char* kAsciiFilter =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
}

SettingsPopup* SettingsPopup::create() {
    return create(s_lastTab);
}

SettingsPopup* SettingsPopup::create(SettingsTab initialTab) {
    auto ret = new SettingsPopup();
    if (ret && ret->init(initialTab)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool SettingsPopup::init(SettingsTab initialTab) {
    if (!Popup::init(450.f, 295.f)) return false;
    this->setTitle("Mod Settings");
    this->setID("SettingsPopup"_spr);

    auto size = m_mainLayer->getContentSize();
    float centerX = size.width / 2.f;

    // 内容区半透明底板
    auto panelBg = CCScale9Sprite::create("square02b_001.png");
    panelBg->setColor({ 0, 0, 0 });
    panelBg->setOpacity(75);
    panelBg->setContentSize({ 420.f, 212.f });
    panelBg->setPosition({ centerX, 120.f });
    m_mainLayer->addChild(panelBg, 0);

    // 顶部 Tab 菜单容器
    m_tabMenu = CCMenu::create();
    m_tabMenu->setPosition({ 0.f, 0.f });
    m_mainLayer->addChild(m_tabMenu, 2);

    // Tab 内容容器
    m_tabContent = CCNode::create();
    m_tabContent->setContentSize(size);
    m_tabContent->setPosition({ 0.f, 0.f });
    m_mainLayer->addChild(m_tabContent, 1);

    m_currentTab = initialTab;
    s_lastTab = initialTab;
    rebuildTabBar();
    switchTab(initialTab);

    this->scheduleUpdate();
    return true;
}

void SettingsPopup::clearTabPointers() {
    m_circleScaleSlider = nullptr;
    m_circleScaleInput = nullptr;
    m_markerTextSlider = nullptr;
    m_markerTextInput = nullptr;
    m_hudTextSlider = nullptr;
    m_hudTextInput = nullptr;
    m_hudSpacingSlider = nullptr;
    m_hudSpacingInput = nullptr;
    m_precTextSlider = nullptr;
    m_precTextInput = nullptr;
    m_precSpacingSlider = nullptr;
    m_precSpacingInput = nullptr;

    m_labelIdInput = nullptr;
    m_labelMinInput = nullptr;
    m_labelMaxInput = nullptr;
    m_labelTextInput = nullptr;
    m_labelAudioInput = nullptr;
    m_labelMinLbl = nullptr;
    m_labelMaxLbl = nullptr;
    m_labelIfToggle = nullptr;
    m_labelHudToggle = nullptr;
    m_labelColorSprite = nullptr;

    m_winIfInput = nullptr;
    m_winValInput = nullptr;
    m_winTextInput = nullptr;
    m_winColorSprite = nullptr;

    m_calcBtnLabel = nullptr;

    m_tpsInput = nullptr;
    m_respawnInput = nullptr;
    m_targetTimeInput = nullptr;
    m_nerveInput = nullptr;
    m_fatigueInput = nullptr;
    m_cpsInput = nullptr;
}

void SettingsPopup::rebuildTabBar() {
    if (!m_tabMenu) return;
    m_tabMenu->removeAllChildren();

    struct TabDef {
        SettingsTab id;
        const char* name;
    };
    const TabDef tabs[] = {
        { SettingsTab::Display,   "Display"   },
        { SettingsTab::Labels,    "Labels"    },
        { SettingsTab::Windows,   "Windows"   },
        { SettingsTab::Precision, "Precision" },
        { SettingsTab::LStarCalc, "L* Calc"   },
    };

    float centerX = m_mainLayer->getContentSize().width / 2.f;
    constexpr float btnW = 78.f;
    constexpr float btnH = 22.f;
    constexpr float gap = 6.f;
    constexpr int count = 5;
    float totalW = count * btnW + (count - 1) * gap;
    float startX = centerX - totalW / 2.f + btnW / 2.f;
    float tabY = 242.f;

    for (int i = 0; i < count; ++i) {
        bool active = (tabs[i].id == m_currentTab);

        auto container = CCNode::create();
        container->setContentSize({ btnW, btnH });

        auto bg = CCScale9Sprite::create(active ? "GJ_button_01.png" : "GJ_button_04.png");
        bg->setContentSize({ btnW, btnH });
        bg->setPosition({ btnW / 2.f, btnH / 2.f });
        if (!active) {
            bg->setOpacity(190);
        }
        container->addChild(bg);

        auto lbl = geode::Label::create(tabs[i].name, "bigFont.fnt");
        lbl->setLimitLabelWidth(btnW - 10.f, 0.36f, 0.2f);
        lbl->setPosition({ btnW / 2.f, btnH / 2.f + 1.f });
        lbl->setColor(active ? ccColor3B{ 255, 255, 255 } : ccColor3B{ 210, 210, 210 });
        container->addChild(lbl);

        auto btn = CCMenuItemSpriteExtra::create(
            container,
            this,
            menu_selector(SettingsPopup::onTabClicked)
        );
        btn->setTag(static_cast<int>(tabs[i].id));
        btn->setPosition({ startX + i * (btnW + gap), tabY });
        m_tabMenu->addChild(btn);
    }
}

void SettingsPopup::onTabClicked(CCObject* sender) {
    auto btn = typeinfo_cast<CCMenuItemSpriteExtra*>(sender);
    if (!btn) return;
    auto target = static_cast<SettingsTab>(btn->getTag());
    if (target == m_currentTab) return;
    switchTab(target);
}

void SettingsPopup::switchTab(SettingsTab tab) {
    // 离开 L* Calc 页时确保输入已落盘
    if (m_currentTab == SettingsTab::LStarCalc && m_tpsInput) {
        saveCalcInputs();
    }

    m_currentTab = tab;
    s_lastTab = tab;

    clearTabPointers();
    if (m_tabContent) {
        m_tabContent->removeAllChildrenWithCleanup(true);
    }

    rebuildTabBar();

    switch (tab) {
    case SettingsTab::Display:   buildDisplayTab();   break;
    case SettingsTab::Labels:    buildLabelsTab();    break;
    case SettingsTab::Windows:   buildWindowsTab();   break;
    case SettingsTab::Precision: buildPrecisionTab(); break;
    case SettingsTab::LStarCalc: buildCalcTab();      break;
    }
}

// Tab 1: Display（圆圈 / 标记数字 / HUD 文字与行距 / Precision 文字）
void SettingsPopup::buildDisplayTab() {
    float centerX = m_mainLayer->getContentSize().width / 2.f;
    auto menu = CCMenu::create();
    menu->setPosition({ 0.f, 0.f });
    m_tabContent->addChild(menu);

    auto addRow = [&](const char* title, float y, float& targetVar, float minV, float maxV,
        float hardMin, const char* fmtStr, Slider*& outSlider, TextInput*& outInput) {
            auto lbl = geode::Label::create(title, "bigFont.fnt");
            lbl->setAnchorPoint({ 1.f, 0.5f });
            lbl->setLimitLabelWidth(134.f, 0.34f, 0.2f);
            lbl->setPosition({ 158.f, y });
            m_tabContent->addChild(lbl);

            auto slider = Slider::create(this, menu_selector(SettingsPopup::onDisplaySliderChanged), 0.62f);
            slider->setPosition({ 248.f, y });
            slider->setValue(toNorm(targetVar, minV, maxV));
            slider->updateBar();
            m_tabContent->addChild(slider);
            outSlider = slider;

            auto input = TextInput::create(68.f, "0.0");
            input->setPosition({ 378.f, y });
            input->setScale(0.72f);
            input->setFilter("0123456789.");
            input->setNumericMode(true);
            input->setString(fmt::format(fmt::runtime(fmtStr), targetVar));
            input->setCallback([this, &targetVar, minV, maxV, hardMin, slider](std::string const& text) {
                if (m_suppressCallbacks || text.empty()) return;
                try {
                    float val = std::stof(text);
                    if (!std::isfinite(val) || val < hardMin) return;
                    targetVar = val;
                    if (slider) {
                        slider->setValue(toNorm(val, minV, maxV));
                        slider->updateBar();
                    }
                    saveVisualSettings();
                    triggerVisualRefresh();
                }
                catch (...) {}
                });
            m_tabContent->addChild(input);
            outInput = input;
        };

    addRow("Circle Size:", 206.f, g_markerCircleScale, kCircleMin, kCircleMax, 0.05f, "{:.2f}", m_circleScaleSlider, m_circleScaleInput);
    addRow("Marker Num Size:", 179.f, g_markerTextScale, kMarkerTextMin, kMarkerTextMax, 0.05f, "{:.2f}", m_markerTextSlider, m_markerTextInput);
    addRow("HUD Text Size:", 152.f, g_hudTextScale, kHudTextMin, kHudTextMax, 0.05f, "{:.2f}", m_hudTextSlider, m_hudTextInput);
    addRow("HUD Line Spacing:", 125.f, g_hudLineSpacing, kHudSpaceMin, kHudSpaceMax, 2.0f, "{:.1f}", m_hudSpacingSlider, m_hudSpacingInput);
    addRow("Precision Size:", 98.f, g_precTextScale, kPrecTextMin, kPrecTextMax, 0.05f, "{:.2f}", m_precTextSlider, m_precTextInput);
    addRow("Prec. Spacing:", 71.f, g_precLineSpacing, kPrecSpaceMin, kPrecSpaceMax, 2.0f, "{:.1f}", m_precSpacingSlider, m_precSpacingInput);

    auto resetSpr = ButtonSprite::create("Reset Visuals", "goldFont.fnt", "GJ_button_05.png", 0.8f);
    resetSpr->setScale(0.62f);
    auto resetBtn = CCMenuItemSpriteExtra::create(resetSpr, this, menu_selector(SettingsPopup::onResetVisuals));
    resetBtn->setPosition({ centerX, 33.f });
    menu->addChild(resetBtn);
}

void SettingsPopup::onDisplaySliderChanged(CCObject*) {
    m_suppressCallbacks = true;

    auto syncOne = [](Slider* slider, TextInput* input, float& targetVar, float minV, float maxV, const char* fmtStr) {
        if (!slider) return;
        float val = fromNorm(slider->getValue(), minV, maxV);
        if (std::abs(val - targetVar) > 1e-4f) {
            targetVar = val;
            if (input) input->setString(fmt::format(fmt::runtime(fmtStr), val));
        }
        };

    syncOne(m_circleScaleSlider, m_circleScaleInput, g_markerCircleScale, kCircleMin, kCircleMax, "{:.2f}");
    syncOne(m_markerTextSlider, m_markerTextInput, g_markerTextScale, kMarkerTextMin, kMarkerTextMax, "{:.2f}");
    syncOne(m_hudTextSlider, m_hudTextInput, g_hudTextScale, kHudTextMin, kHudTextMax, "{:.2f}");
    syncOne(m_hudSpacingSlider, m_hudSpacingInput, g_hudLineSpacing, kHudSpaceMin, kHudSpaceMax, "{:.1f}");
    syncOne(m_precTextSlider, m_precTextInput, g_precTextScale, kPrecTextMin, kPrecTextMax, "{:.2f}");
    syncOne(m_precSpacingSlider, m_precSpacingInput, g_precLineSpacing, kPrecSpaceMin, kPrecSpaceMax, "{:.1f}");

    m_suppressCallbacks = false;

    saveVisualSettings();
    triggerVisualRefresh();
}

void SettingsPopup::onResetVisuals(CCObject*) {
    resetVisualSettingsToDefault();
    if (m_currentTab == SettingsTab::Display) {
        m_suppressCallbacks = true;
        if (m_circleScaleSlider) { m_circleScaleSlider->setValue(toNorm(g_markerCircleScale, kCircleMin, kCircleMax)); m_circleScaleSlider->updateBar(); }
        if (m_circleScaleInput)  m_circleScaleInput->setString(fmt::format("{:.2f}", g_markerCircleScale));
        if (m_markerTextSlider) { m_markerTextSlider->setValue(toNorm(g_markerTextScale, kMarkerTextMin, kMarkerTextMax)); m_markerTextSlider->updateBar(); }
        if (m_markerTextInput)   m_markerTextInput->setString(fmt::format("{:.2f}", g_markerTextScale));
        if (m_hudTextSlider) { m_hudTextSlider->setValue(toNorm(g_hudTextScale, kHudTextMin, kHudTextMax)); m_hudTextSlider->updateBar(); }
        if (m_hudTextInput)      m_hudTextInput->setString(fmt::format("{:.2f}", g_hudTextScale));
        if (m_hudSpacingSlider) { m_hudSpacingSlider->setValue(toNorm(g_hudLineSpacing, kHudSpaceMin, kHudSpaceMax)); m_hudSpacingSlider->updateBar(); }
        if (m_hudSpacingInput)   m_hudSpacingInput->setString(fmt::format("{:.1f}", g_hudLineSpacing));
        if (m_precTextSlider) { m_precTextSlider->setValue(toNorm(g_precTextScale, kPrecTextMin, kPrecTextMax)); m_precTextSlider->updateBar(); }
        if (m_precTextInput)     m_precTextInput->setString(fmt::format("{:.2f}", g_precTextScale));
        if (m_precSpacingSlider) { m_precSpacingSlider->setValue(toNorm(g_precLineSpacing, kPrecSpaceMin, kPrecSpaceMax)); m_precSpacingSlider->updateBar(); }
        if (m_precSpacingInput)  m_precSpacingInput->setString(fmt::format("{:.1f}", g_precLineSpacing));
        m_suppressCallbacks = false;
    }
}

// Tab 2: Labels
void SettingsPopup::buildLabelsTab() {
    auto menu = CCMenu::create();
    menu->setPosition({ 0.f, 0.f });
    m_tabContent->addChild(menu);

    // 第一行：ID (< [input] >) | I/F 开关 | Show HUD 开关
    auto idLbl = geode::Label::create("ID:", "bigFont.fnt");
    idLbl->setScale(0.38f);
    idLbl->setPosition({ 48.f, 198.f });
    m_tabContent->addChild(idLbl);

    auto prevSpr = CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png");
    prevSpr->setScale(0.42f);
    auto prevBtn = CCMenuItemSpriteExtra::create(prevSpr, this, menu_selector(SettingsPopup::onLabelPrevId));
    prevBtn->setPosition({ 76.f, 198.f });
    menu->addChild(prevBtn);

    m_labelIdInput = TextInput::create(52.f, "0");
    m_labelIdInput->setPosition({ 112.f, 198.f });
    m_labelIdInput->setScale(0.8f);
    m_labelIdInput->setFilter("0123456789");
    m_labelIdInput->setNumericMode(true);
    m_labelIdInput->setString(std::to_string(m_lastLabelId));
    m_labelIdInput->setCallback([this](std::string const& text) {
        if (m_suppressCallbacks || text.empty()) return;
        try { m_lastLabelId = std::clamp(std::stoi(text), 0, 99); }
        catch (...) {}
        this->loadLabelPreset();
        });
    m_tabContent->addChild(m_labelIdInput);

    auto nextSpr = CCSprite::createWithSpriteFrameName("edit_rightBtn_001.png");
    nextSpr->setScale(0.42f);
    auto nextBtn = CCMenuItemSpriteExtra::create(nextSpr, this, menu_selector(SettingsPopup::onLabelNextId));
    nextBtn->setPosition({ 148.f, 198.f });
    menu->addChild(nextBtn);

    auto ifLbl = geode::Label::create("I/F:", "bigFont.fnt");
    ifLbl->setScale(0.36f);
    ifLbl->setPosition({ 200.f, 198.f });
    m_tabContent->addChild(ifLbl);

    m_labelIfToggle = CCMenuItemToggler::createWithStandardSprites(
        this, menu_selector(SettingsPopup::onLabelIFToggle), 0.62f
    );
    m_labelIfToggle->setPosition({ 232.f, 198.f });
    menu->addChild(m_labelIfToggle);

    auto hudLbl = geode::Label::create("Show HUD:", "bigFont.fnt");
    hudLbl->setScale(0.36f);
    hudLbl->setPosition({ 328.f, 198.f });
    m_tabContent->addChild(hudLbl);

    m_labelHudToggle = CCMenuItemToggler::createWithStandardSprites(
        this, menu_selector(SettingsPopup::onLabelHudToggle), 0.62f
    );
    m_labelHudToggle->setPosition({ 392.f, 198.f });
    menu->addChild(m_labelHudToggle);

    // 第二行：Min / Max 与 Color
    m_labelMinLbl = geode::Label::create("Min Win:", "bigFont.fnt");
    m_labelMinLbl->setAnchorPoint({ 1.f, 0.5f });
    m_labelMinLbl->setScale(0.35f);
    m_labelMinLbl->setPosition({ 94.f, 158.f });
    m_tabContent->addChild(m_labelMinLbl);

    m_labelMinInput = TextInput::create(70.f, "0");
    m_labelMinInput->setPosition({ 134.f, 158.f });
    m_labelMinInput->setScale(0.8f);
    m_labelMinInput->setFilter("0123456789./");
    m_labelMinInput->setNumericMode(true);
    m_labelMinInput->setCallback([this](std::string const& text) {
        if (m_suppressCallbacks) return;
        if (m_labelCurrentUseIF) m_labelMinIFStr = text;
        else m_labelMinWinStr = text;
        this->saveLabelPreset();
        });
    m_tabContent->addChild(m_labelMinInput);

    m_labelMaxLbl = geode::Label::create("Max Win:", "bigFont.fnt");
    m_labelMaxLbl->setAnchorPoint({ 1.f, 0.5f });
    m_labelMaxLbl->setScale(0.35f);
    m_labelMaxLbl->setPosition({ 242.f, 158.f });
    m_tabContent->addChild(m_labelMaxLbl);

    m_labelMaxInput = TextInput::create(74.f, "999999");
    m_labelMaxInput->setPosition({ 284.f, 158.f });
    m_labelMaxInput->setScale(0.8f);
    m_labelMaxInput->setFilter("0123456789./");
    m_labelMaxInput->setNumericMode(true);
    m_labelMaxInput->setCallback([this](std::string const& text) {
        if (m_suppressCallbacks) return;
        if (m_labelCurrentUseIF) m_labelMaxIFStr = text;
        else m_labelMaxWinStr = text;
        this->saveLabelPreset();
        });
    m_tabContent->addChild(m_labelMaxInput);

    auto colorLbl = geode::Label::create("Color:", "bigFont.fnt");
    colorLbl->setScale(0.35f);
    colorLbl->setPosition({ 358.f, 158.f });
    m_tabContent->addChild(colorLbl);

    m_labelColorSprite = CCSprite::createWithSpriteFrameName("GJ_colorBtn_001.png");
    auto colorWrapper = CCNode::create();
    colorWrapper->setContentSize(m_labelColorSprite->getContentSize());
    m_labelColorSprite->setPosition(colorWrapper->getContentSize() / 2.f);
    colorWrapper->addChild(m_labelColorSprite);
    colorWrapper->setScale(0.6f);

    auto colorBtn = CCMenuItemSpriteExtra::create(colorWrapper, this, menu_selector(SettingsPopup::onLabelColorBtn));
    colorBtn->setPosition({ 402.f, 158.f });
    menu->addChild(colorBtn);

    // 第三行：HUD Text
    auto txtLbl = geode::Label::create("HUD Text:", "bigFont.fnt");
    txtLbl->setAnchorPoint({ 1.f, 0.5f });
    txtLbl->setScale(0.35f);
    txtLbl->setPosition({ 102.f, 118.f });
    m_tabContent->addChild(txtLbl);

    m_labelTextInput = TextInput::create(300.f, "HUD Display Text");
    m_labelTextInput->setPosition({ 262.f, 118.f });
    m_labelTextInput->setScale(0.85f);
    m_labelTextInput->setFilter(kAsciiFilter);
    m_labelTextInput->setCallback([this](std::string const&) {
        if (!m_suppressCallbacks) this->saveLabelPreset();
        });
    m_tabContent->addChild(m_labelTextInput);

    // 第四行：Audio Path + Browse
    auto audioLbl = geode::Label::create("Audio:", "bigFont.fnt");
    audioLbl->setAnchorPoint({ 1.f, 0.5f });
    audioLbl->setScale(0.35f);
    audioLbl->setPosition({ 102.f, 78.f });
    m_tabContent->addChild(audioLbl);

    m_labelAudioInput = TextInput::create(220.f, "Audio Path");
    m_labelAudioInput->setPosition({ 220.f, 78.f });
    m_labelAudioInput->setScale(0.85f);
    m_labelAudioInput->setFilter(kAsciiFilter);
    m_labelAudioInput->setCallback([this](std::string const&) {
        if (!m_suppressCallbacks) this->saveLabelPreset();
        });
    m_tabContent->addChild(m_labelAudioInput);

    auto browseSpr = ButtonSprite::create("Browse");
    browseSpr->setScale(0.62f);
    auto browseBtn = CCMenuItemSpriteExtra::create(browseSpr, this, menu_selector(SettingsPopup::onLabelBrowseAudio));
    browseBtn->setPosition({ 374.f, 78.f });
    menu->addChild(browseBtn);

    // 第五行：Apply to Wins / Reset All
    auto applySpr = ButtonSprite::create("Apply to Wins");
    applySpr->setScale(0.58f);
    auto applyBtn = CCMenuItemSpriteExtra::create(applySpr, this, menu_selector(SettingsPopup::onLabelApplyToWins));
    applyBtn->setPosition({ 148.f, 34.f });
    menu->addChild(applyBtn);

    auto resetSpr = ButtonSprite::create("Reset All Labels", "goldFont.fnt", "GJ_button_05.png", 0.8f);
    resetSpr->setScale(0.58f);
    auto resetBtn = CCMenuItemSpriteExtra::create(resetSpr, this, menu_selector(SettingsPopup::onLabelResetAll));
    resetBtn->setPosition({ 305.f, 34.f });
    menu->addChild(resetBtn);

    loadLabelPreset();
}

void SettingsPopup::onLabelPrevId(CCObject*) {
    m_lastLabelId = std::max(0, m_lastLabelId - 1);
    m_suppressCallbacks = true;
    if (m_labelIdInput) m_labelIdInput->setString(std::to_string(m_lastLabelId));
    m_suppressCallbacks = false;
    loadLabelPreset();
}

void SettingsPopup::onLabelNextId(CCObject*) {
    m_lastLabelId = std::min(99, m_lastLabelId + 1);
    m_suppressCallbacks = true;
    if (m_labelIdInput) m_labelIdInput->setString(std::to_string(m_lastLabelId));
    m_suppressCallbacks = false;
    loadLabelPreset();
}

void SettingsPopup::loadLabelPreset() {
    std::string idStr = m_labelIdInput ? m_labelIdInput->getString() : std::to_string(m_lastLabelId);
    if (idStr.empty()) return;

    m_suppressCallbacks = true;
    if (g_labelPresets.contains(idStr)) {
        auto& p = g_labelPresets[idStr];
        m_labelCurrentUseIF = p.useIF;
        if (m_labelIfToggle) m_labelIfToggle->toggle(m_labelCurrentUseIF);

        m_labelMinWinStr = p.minWindowStr;
        m_labelMaxWinStr = p.maxWindowStr;
        m_labelMinIFStr = p.minIFStr;
        m_labelMaxIFStr = p.maxIFStr;

        if (m_labelCurrentUseIF) {
            if (m_labelMinLbl) m_labelMinLbl->setString("Min I/F:");
            if (m_labelMaxLbl) m_labelMaxLbl->setString("Max I/F:");
            if (m_labelMinInput) m_labelMinInput->setString(p.minIFStr);
            if (m_labelMaxInput) m_labelMaxInput->setString(p.maxIFStr);
        }
        else {
            if (m_labelMinLbl) m_labelMinLbl->setString("Min Win:");
            if (m_labelMaxLbl) m_labelMaxLbl->setString("Max Win:");
            if (m_labelMinInput) m_labelMinInput->setString(p.minWindowStr);
            if (m_labelMaxInput) m_labelMaxInput->setString(p.maxWindowStr);
        }

        if (m_labelTextInput) m_labelTextInput->setString(p.text);
        if (m_labelAudioInput) m_labelAudioInput->setString(p.audioPath);

        m_labelCurrentColor = p.color;
        if (m_labelColorSprite) {
            m_labelColorSprite->setColor({
                static_cast<GLubyte>(p.color.r * 255),
                static_cast<GLubyte>(p.color.g * 255),
                static_cast<GLubyte>(p.color.b * 255)
                });
            m_labelColorSprite->setOpacity(static_cast<GLubyte>(p.color.a * 255));
        }

        m_labelCurrentShowInHud = p.showInHud;
        if (m_labelHudToggle) m_labelHudToggle->toggle(m_labelCurrentShowInHud);
    }
    else {
        m_labelCurrentUseIF = false;
        if (m_labelIfToggle) m_labelIfToggle->toggle(false);

        m_labelMinWinStr = idStr;
        m_labelMaxWinStr = idStr;
        m_labelMinIFStr = "";
        m_labelMaxIFStr = "";

        if (m_labelMinLbl) m_labelMinLbl->setString("Min Win:");
        if (m_labelMaxLbl) m_labelMaxLbl->setString("Max Win:");
        if (m_labelMinInput) m_labelMinInput->setString(idStr);
        if (m_labelMaxInput) m_labelMaxInput->setString(idStr);
        if (m_labelTextInput) m_labelTextInput->setString(idStr);
        if (m_labelAudioInput) m_labelAudioInput->setString("");

        m_labelCurrentColor = { 1.f, 1.f, 1.f, 1.f };
        if (m_labelColorSprite) {
            m_labelColorSprite->setColor({ 255, 255, 255 });
            m_labelColorSprite->setOpacity(255);
        }

        m_labelCurrentShowInHud = false;
        if (m_labelHudToggle) m_labelHudToggle->toggle(false);
    }
    m_suppressCallbacks = false;
}

void SettingsPopup::saveLabelPreset() {
    std::string idStr = m_labelIdInput ? m_labelIdInput->getString() : "";
    if (idStr.empty()) return;

    LabelPreset p;
    try { p.id = std::stoi(idStr); }
    catch (...) { p.id = 0; }

    p.useIF = m_labelCurrentUseIF;
    p.minWindowStr = m_labelMinWinStr;
    p.maxWindowStr = m_labelMaxWinStr;
    p.minIFStr = m_labelMinIFStr;
    p.maxIFStr = m_labelMaxIFStr;
    p.text = m_labelTextInput ? m_labelTextInput->getString() : "";
    p.audioPath = m_labelAudioInput ? m_labelAudioInput->getString() : "";
    p.color = m_labelCurrentColor;
    p.showInHud = m_labelCurrentShowInHud;
    p.updateBounds();

    g_labelPresets[idStr] = p;
    saveSettings();
    triggerHUDRefresh();
}

void SettingsPopup::onLabelIFToggle(CCObject* sender) {
    if (auto toggle = typeinfo_cast<CCMenuItemToggler*>(sender)) {
        if (m_labelCurrentUseIF) {
            if (m_labelMinInput) m_labelMinIFStr = m_labelMinInput->getString();
            if (m_labelMaxInput) m_labelMaxIFStr = m_labelMaxInput->getString();
        }
        else {
            if (m_labelMinInput) m_labelMinWinStr = m_labelMinInput->getString();
            if (m_labelMaxInput) m_labelMaxWinStr = m_labelMaxInput->getString();
        }

        m_labelCurrentUseIF = !toggle->isToggled();

        m_suppressCallbacks = true;
        if (m_labelCurrentUseIF) {
            if (m_labelMinLbl) m_labelMinLbl->setString("Min I/F:");
            if (m_labelMaxLbl) m_labelMaxLbl->setString("Max I/F:");
            if (m_labelMinInput) m_labelMinInput->setString(m_labelMinIFStr.empty() ? "1" : m_labelMinIFStr);
            if (m_labelMaxInput) m_labelMaxInput->setString(m_labelMaxIFStr.empty() ? "999999" : m_labelMaxIFStr);
        }
        else {
            if (m_labelMinLbl) m_labelMinLbl->setString("Min Win:");
            if (m_labelMaxLbl) m_labelMaxLbl->setString("Max Win:");
            if (m_labelMinInput) m_labelMinInput->setString(m_labelMinWinStr.empty() ? "0" : m_labelMinWinStr);
            if (m_labelMaxInput) m_labelMaxInput->setString(m_labelMaxWinStr.empty() ? "999999" : m_labelMaxWinStr);
        }
        m_suppressCallbacks = false;

        saveLabelPreset();
    }
}

void SettingsPopup::onLabelHudToggle(CCObject* sender) {
    if (auto toggle = typeinfo_cast<CCMenuItemToggler*>(sender)) {
        m_labelCurrentShowInHud = !toggle->isToggled();
        saveLabelPreset();
    }
}

void SettingsPopup::onLabelColorBtn(CCObject*) {
    Ref<SettingsPopup> safeThis = this;
    auto popup = geode::ColorPickPopup::create({
        static_cast<GLubyte>(m_labelCurrentColor.r * 255),
        static_cast<GLubyte>(m_labelCurrentColor.g * 255),
        static_cast<GLubyte>(m_labelCurrentColor.b * 255),
        static_cast<GLubyte>(m_labelCurrentColor.a * 255)
        });
    popup->setCallback([safeThis](ccColor4B color) {
        if (!safeThis || !safeThis->getParent() || safeThis->m_currentTab != SettingsTab::Labels) return;
        safeThis->m_labelCurrentColor = {
            color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f
        };
        if (safeThis->m_labelColorSprite) {
            safeThis->m_labelColorSprite->setColor({ color.r, color.g, color.b });
            safeThis->m_labelColorSprite->setOpacity(color.a);
        }
        safeThis->saveLabelPreset();
        });
    popup->show();
}

void SettingsPopup::onLabelBrowseAudio(CCObject*) {
#if defined(GEODE_IS_ANDROID) || defined(GEODE_IS_IOS)
    Ref<SettingsPopup> safeThis = this;
    auto popup = MobileAudioPickerPopup::create([safeThis](std::string const& selectedPath) {
        if (safeThis && safeThis->getParent() &&
            safeThis->m_currentTab == SettingsTab::Labels && safeThis->m_labelAudioInput) {
            safeThis->m_labelAudioInput->setString(selectedPath);
            safeThis->saveLabelPreset();
        }
        });
    if (popup) popup->showInstant();
#else
    file::FilePickOptions options;
    options.filters.push_back({ "Audio Files", { "*.ogg", "*.mp3", "*.wav" } });

    Ref<SettingsPopup> safeThis = this;
    async::spawn(
        file::pick(file::PickMode::OpenFile, options),
        [safeThis](Result<std::optional<asp::fs::Path>> result) {
            if (!result.isOk()) return;
            auto opt = result.unwrap();
            if (!opt.has_value()) return;
            if (safeThis && safeThis->getParent() &&
                safeThis->m_currentTab == SettingsTab::Labels && safeThis->m_labelAudioInput) {
                safeThis->m_labelAudioInput->setString(opt.value().string());
                safeThis->saveLabelPreset();
            }
        }
    );
#endif
}

void SettingsPopup::onLabelApplyToWins(CCObject*) {
    if (!m_labelMinInput || !m_labelMaxInput) return;
    std::string minStr = m_labelMinInput->getString();
    std::string maxStr = m_labelMaxInput->getString();
    int count = 0;

    if (m_labelCurrentUseIF) {
        int minIF = std::max(1, static_cast<int>(std::round(parseWindowExpr(minStr, 1.0))));
        int maxIF = static_cast<int>(std::round(parseWindowExpr(maxStr, 999999.0)));

        if (minIF > maxIF) {
            auto alert = FLAlertLayer::create("Error", "Min value cannot be greater than Max value.", "OK");
            alert->show(); stopAlertAnimation(alert);
            return;
        }
        if (maxIF - minIF + 1 > 1000) {
            auto alert = FLAlertLayer::create("Error", "Range too large!\nTotal count cannot exceed 1000", "OK");
            alert->show(); stopAlertAnimation(alert);
            return;
        }

        for (auto& [key, preset] : g_windowPresets) {
            if (preset.ifCount >= minIF && preset.ifCount <= maxIF) {
                preset.color = m_labelCurrentColor;
                count++;
            }
        }

        std::set<double> existingWindows;
        for (auto const& [key, preset] : g_windowPresets) existingWindows.insert(preset.window);
        if (existingWindows.empty()) existingWindows.insert(1.0);

        for (int k = minIF; k <= maxIF; ++k) {
            for (double w : existingWindows) {
                auto key = makeWindowPresetKey(k, w);
                if (!g_windowPresets.contains(key)) {
                    FrameWindowPreset p;
                    p.ifCount = k;
                    p.window = w;
                    p.color = m_labelCurrentColor;
                    g_windowPresets[key] = p;
                    count++;
                }
            }
        }

        saveSettings();
        triggerHUDRefresh();
        auto alert = FLAlertLayer::create("Success", fmt::format("Applied color to {} presets under I/F range ({} - {})!", count, minIF, maxIF), "OK");
        alert->show(); stopAlertAnimation(alert);
    }
    else {
        double minWin = std::max(0.0, parseWindowExpr(minStr, 0.0));
        double maxWin = parseWindowExpr(maxStr, 999999.0);

        if (minWin > maxWin) {
            auto alert = FLAlertLayer::create("Error", "Min value cannot be greater than Max value.", "OK");
            alert->show(); stopAlertAnimation(alert);
            return;
        }
        if (maxWin - minWin > 1000.0) {
            auto alert = FLAlertLayer::create("Error", "Range too large!\nTotal span cannot exceed 1000", "OK");
            alert->show(); stopAlertAnimation(alert);
            return;
        }

        for (auto& [key, preset] : g_windowPresets) {
            if (preset.window >= (minWin - 1e-6) && preset.window <= (maxWin + 1e-6)) {
                preset.color = m_labelCurrentColor;
                count++;
            }
        }

        std::set<double> targetWindows = { minWin, maxWin };
        int startInt = static_cast<int>(std::ceil(minWin - 1e-6));
        int endInt = static_cast<int>(std::floor(maxWin + 1e-6));
        for (int i = startInt; i <= endInt; ++i) targetWindows.insert(static_cast<double>(i));

        std::set<int> existingIFs = { 1 };
        for (auto const& [key, preset] : g_windowPresets) existingIFs.insert(preset.ifCount);

        for (int k : existingIFs) {
            for (double w : targetWindows) {
                if (w < 0.0) continue;
                auto key = makeWindowPresetKey(k, w);
                if (!g_windowPresets.contains(key)) {
                    FrameWindowPreset p;
                    p.ifCount = k;
                    p.window = w;
                    p.color = m_labelCurrentColor;
                    g_windowPresets[key] = p;
                    count++;
                }
            }
        }

        saveSettings();
        triggerHUDRefresh();
        auto alert = FLAlertLayer::create("Success", fmt::format("Applied color to {} presets across all I/F values for Windows ({:.2f} - {:.2f})!", count, minWin, maxWin), "OK");
        alert->show(); stopAlertAnimation(alert);
    }
}

void SettingsPopup::onLabelResetAll(CCObject*) {
    Ref<SettingsPopup> safeThis = this;
    auto alert = geode::createQuickPopup(
        "Reset All Labels",
        "Are you sure you want to clear <cr>ALL label presets</c>?",
        "Cancel", "Reset",
        [safeThis](auto, bool btn2) {
            if (!btn2) return;
            g_labelPresets.clear();
            saveSettings();
            triggerHUDRefresh();
            if (safeThis && safeThis->getParent() && safeThis->m_currentTab == SettingsTab::Labels) {
                safeThis->loadLabelPreset();
            }
            auto ok = FLAlertLayer::create("Success", "All label presets have been cleared.", "OK");
            ok->show(); stopAlertAnimation(ok);
        }
    );
    stopAlertAnimation(alert);
}

// Tab 3: Windows
void SettingsPopup::buildWindowsTab() {
    float centerX = m_mainLayer->getContentSize().width / 2.f;
    auto menu = CCMenu::create();
    menu->setPosition({ 0.f, 0.f });
    m_tabContent->addChild(menu);

    // 第一行：I/F 与 Win
    auto ifLbl = geode::Label::create("I/F:", "bigFont.fnt");
    ifLbl->setScale(0.38f);
    ifLbl->setPosition({ 82.f, 186.f });
    m_tabContent->addChild(ifLbl);

    m_winIfInput = TextInput::create(52.f, "1");
    m_winIfInput->setPosition({ 148.f, 186.f });
    m_winIfInput->setScale(0.82f);
    m_winIfInput->setFilter("0123456789");
    m_winIfInput->setNumericMode(true);
    m_winIfInput->setString(std::to_string(m_lastWinIF));
    m_winIfInput->setCallback([this](std::string const& text) {
        if (m_suppressCallbacks || text.empty()) return;
        try { m_lastWinIF = std::max(1, std::stoi(text)); }
        catch (...) {}
        this->loadWindowPreset();
        });
    m_tabContent->addChild(m_winIfInput);

    auto winLbl = geode::Label::create("Win:", "bigFont.fnt");
    winLbl->setScale(0.38f);
    winLbl->setPosition({ 256.f, 186.f });
    m_tabContent->addChild(winLbl);

    m_winValInput = TextInput::create(82.f, "1");
    m_winValInput->setPosition({ 318.f, 186.f });
    m_winValInput->setScale(0.82f);
    m_winValInput->setFilter("0123456789./");
    m_winValInput->setNumericMode(true);
    m_winValInput->setString(m_lastWinStr);
    m_winValInput->setCallback([this](std::string const& text) {
        if (m_suppressCallbacks) return;
        m_lastWinStr = text;
        this->loadWindowPreset();
        });
    m_tabContent->addChild(m_winValInput);

    // 第二行：Custom Text
    auto textLbl = geode::Label::create("Custom Text:", "bigFont.fnt");
    textLbl->setAnchorPoint({ 1.f, 0.5f });
    textLbl->setScale(0.36f);
    textLbl->setPosition({ 134.f, 138.f });
    m_tabContent->addChild(textLbl);

    m_winTextInput = TextInput::create(260.f, "Custom Text (Default: Win)");
    m_winTextInput->setPosition({ 274.f, 138.f });
    m_winTextInput->setScale(0.85f);
    m_winTextInput->setFilter(kAsciiFilter);
    m_winTextInput->setCallback([this](std::string const&) {
        if (!m_suppressCallbacks) this->saveWindowPreset();
        });
    m_tabContent->addChild(m_winTextInput);

    // 第三行：Circle Color
    auto colorLbl = geode::Label::create("Circle Color:", "bigFont.fnt");
    colorLbl->setAnchorPoint({ 1.f, 0.5f });
    colorLbl->setScale(0.4f);
    colorLbl->setPosition({ centerX + 10.f, 92.f });
    m_tabContent->addChild(colorLbl);

    m_winColorSprite = CCSprite::createWithSpriteFrameName("GJ_colorBtn_001.png");
    auto colorWrapper = CCNode::create();
    colorWrapper->setContentSize(m_winColorSprite->getContentSize());
    m_winColorSprite->setPosition(colorWrapper->getContentSize() / 2.f);
    colorWrapper->addChild(m_winColorSprite);
    colorWrapper->setScale(0.68f);

    auto colorBtn = CCMenuItemSpriteExtra::create(colorWrapper, this, menu_selector(SettingsPopup::onWinColorBtn));
    colorBtn->setPosition({ centerX + 52.f, 92.f });
    menu->addChild(colorBtn);

    // 第四行：Reset All Windows
    auto resetSpr = ButtonSprite::create("Reset All Windows", "goldFont.fnt", "GJ_button_05.png", 0.8f);
    resetSpr->setScale(0.6f);
    auto resetBtn = CCMenuItemSpriteExtra::create(resetSpr, this, menu_selector(SettingsPopup::onWinResetAll));
    resetBtn->setPosition({ centerX, 36.f });
    menu->addChild(resetBtn);

    loadWindowPreset();
}

void SettingsPopup::onWinPrevIF(CCObject*) {
    m_lastWinIF = std::max(1, m_lastWinIF - 1);
    m_suppressCallbacks = true;
    if (m_winIfInput) m_winIfInput->setString(std::to_string(m_lastWinIF));
    m_suppressCallbacks = false;
    loadWindowPreset();
}

void SettingsPopup::onWinNextIF(CCObject*) {
    m_lastWinIF += 1;
    m_suppressCallbacks = true;
    if (m_winIfInput) m_winIfInput->setString(std::to_string(m_lastWinIF));
    m_suppressCallbacks = false;
    loadWindowPreset();
}

void SettingsPopup::loadWindowPreset() {
    std::string ifStr = m_winIfInput ? m_winIfInput->getString() : std::to_string(m_lastWinIF);
    std::string winStr = m_winValInput ? m_winValInput->getString() : m_lastWinStr;
    if (winStr.empty()) return;

    int ifVal = 1;
    try { ifVal = std::max(1, std::stoi(ifStr)); }
    catch (...) { ifVal = 1; }
    double winVal = parseWindowExpr(winStr, 1.0);
    std::string key = makeWindowPresetKey(ifVal, winVal);

    m_suppressCallbacks = true;
    if (g_windowPresets.contains(key)) {
        auto& p = g_windowPresets[key];
        m_winCurrentColor = p.color;
        if (m_winTextInput) m_winTextInput->setString(p.customText);
        if (m_winColorSprite) {
            m_winColorSprite->setColor({
                static_cast<GLubyte>(p.color.r * 255),
                static_cast<GLubyte>(p.color.g * 255),
                static_cast<GLubyte>(p.color.b * 255)
                });
            m_winColorSprite->setOpacity(static_cast<GLubyte>(p.color.a * 255));
        }
    }
    else {
        m_winCurrentColor = { 1.f, 1.f, 1.f, 1.f };
        if (m_winTextInput) m_winTextInput->setString("");
        if (m_winColorSprite) {
            m_winColorSprite->setColor({ 255, 255, 255 });
            m_winColorSprite->setOpacity(255);
        }
    }
    m_suppressCallbacks = false;
}

void SettingsPopup::saveWindowPreset() {
    std::string ifStr = m_winIfInput ? m_winIfInput->getString() : "1";
    std::string winStr = m_winValInput ? m_winValInput->getString() : "1";
    if (winStr.empty()) return;

    int ifVal = 1;
    try { ifVal = std::max(1, std::stoi(ifStr)); }
    catch (...) { ifVal = 1; }
    double winVal = parseWindowExpr(winStr, 1.0);

    FrameWindowPreset p;
    p.ifCount = ifVal;
    p.window = winVal;
    p.color = m_winCurrentColor;
    p.customText = m_winTextInput ? m_winTextInput->getString() : "";

    g_windowPresets[makeWindowPresetKey(ifVal, winVal)] = p;
    saveSettings();
}

void SettingsPopup::onWinColorBtn(CCObject*) {
    Ref<SettingsPopup> safeThis = this;
    auto popup = geode::ColorPickPopup::create({
        static_cast<GLubyte>(m_winCurrentColor.r * 255),
        static_cast<GLubyte>(m_winCurrentColor.g * 255),
        static_cast<GLubyte>(m_winCurrentColor.b * 255),
        static_cast<GLubyte>(m_winCurrentColor.a * 255)
        });
    popup->setCallback([safeThis](ccColor4B color) {
        if (!safeThis || !safeThis->getParent() || safeThis->m_currentTab != SettingsTab::Windows) return;
        safeThis->m_winCurrentColor = {
            color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f
        };
        if (safeThis->m_winColorSprite) {
            safeThis->m_winColorSprite->setColor({ color.r, color.g, color.b });
            safeThis->m_winColorSprite->setOpacity(color.a);
        }
        safeThis->saveWindowPreset();
        });
    popup->show();
}

void SettingsPopup::onWinResetAll(CCObject*) {
    Ref<SettingsPopup> safeThis = this;
    auto alert = geode::createQuickPopup(
        "Reset Window Presets",
        "Are you sure you want to reset <cr>ALL</c> window presets?",
        "Cancel", "Reset",
        [safeThis](auto, bool btn2) {
            if (!btn2) return;
            g_windowPresets.clear();
            saveSettings();
            triggerHUDRefresh();
            if (safeThis && safeThis->getParent() && safeThis->m_currentTab == SettingsTab::Windows) {
                safeThis->loadWindowPreset();
            }
            auto ok = FLAlertLayer::create("Success", "All window presets have been reset.", "OK");
            ok->show(); stopAlertAnimation(ok);
        }
    );
    stopAlertAnimation(alert);
}

// Tab 4: Precision
void SettingsPopup::buildPrecisionTab() {
    float centerX = m_mainLayer->getContentSize().width / 2.f;
    auto menu = CCMenu::create();
    menu->setPosition({ 0.f, 0.f });
    m_tabContent->addChild(menu);

    auto addToggle = [&](const char* name, bool state, SEL_MenuHandler selector, float labelX, float toggleX, float y) {
        auto lbl = geode::Label::create(name, "bigFont.fnt");
        lbl->setAnchorPoint({ 0.f, 0.5f });
        lbl->setLimitLabelWidth(toggleX - labelX - 24.f, 0.36f, 0.2f);
        lbl->setPosition({ labelX, y });
        m_tabContent->addChild(lbl);

        auto toggle = CCMenuItemToggler::createWithStandardSprites(this, selector, 0.62f);
        toggle->toggle(state);
        toggle->setPosition({ toggleX, y });
        menu->addChild(toggle);
        };

    addToggle("Base L*", g_showBase, menu_selector(SettingsPopup::onTBase), 40.f, 190.f, 194.f);
    addToggle("Nerve L*", g_showN, menu_selector(SettingsPopup::onTN), 40.f, 190.f, 160.f);
    addToggle("Fatigue L*", g_showF, menu_selector(SettingsPopup::onTF), 40.f, 190.f, 126.f);
    addToggle("CPS L*", g_showC, menu_selector(SettingsPopup::onTC), 40.f, 190.f, 92.f);

    addToggle("Nerve+Fatigue", g_showNF, menu_selector(SettingsPopup::onTNF), 230.f, 405.f, 194.f);
    addToggle("Nerve+CPS", g_showNC, menu_selector(SettingsPopup::onTNC), 230.f, 405.f, 160.f);
    addToggle("Fatigue+CPS", g_showFC, menu_selector(SettingsPopup::onTFC), 230.f, 405.f, 126.f);
    addToggle("Nerve+Fatigue+CPS", g_showNFC, menu_selector(SettingsPopup::onTNFC), 230.f, 405.f, 92.f);

    m_calcBtnLabel = geode::Label::create("Start Calculation", "bigFont.fnt");
    m_calcBtnLabel->setLimitLabelWidth(185.f, 0.42f, 0.25f);

    auto btnContainer = CCNode::create();
    btnContainer->setContentSize({ 200.f, 32.f });

    auto bg = CCScale9Sprite::create("GJ_button_01.png");
    bg->setContentSize({ 200.f, 32.f });
    bg->setPosition(btnContainer->getContentSize() / 2.f);
    btnContainer->addChild(bg);

    m_calcBtnLabel->setPosition(btnContainer->getContentSize() / 2.f);
    btnContainer->addChild(m_calcBtnLabel);

    auto calcBtn = CCMenuItemSpriteExtra::create(btnContainer, this, menu_selector(SettingsPopup::onToggleCalc));
    calcBtn->setPosition({ centerX, 38.f });
    menu->addChild(calcBtn);
}

void SettingsPopup::onTBase(CCObject*) { g_showBase = !g_showBase; saveSettings(); triggerVisualRefresh(); }
void SettingsPopup::onTN(CCObject*) { g_showN = !g_showN;    saveSettings(); triggerVisualRefresh(); }
void SettingsPopup::onTF(CCObject*) { g_showF = !g_showF;    saveSettings(); triggerVisualRefresh(); }
void SettingsPopup::onTC(CCObject*) { g_showC = !g_showC;    saveSettings(); triggerVisualRefresh(); }
void SettingsPopup::onTNF(CCObject*) { g_showNF = !g_showNF;   saveSettings(); triggerVisualRefresh(); }
void SettingsPopup::onTNC(CCObject*) { g_showNC = !g_showNC;   saveSettings(); triggerVisualRefresh(); }
void SettingsPopup::onTFC(CCObject*) { g_showFC = !g_showFC;   saveSettings(); triggerVisualRefresh(); }
void SettingsPopup::onTNFC(CCObject*) { g_showNFC = !g_showNFC;  saveSettings(); triggerVisualRefresh(); }

void SettingsPopup::onToggleCalc(CCObject*) {
    if (g_isCalculating.load()) stopGlobalRecalc();
    else startGlobalRecalc();
}

// Tab 5: L* Calc
void SettingsPopup::buildCalcTab() {
    float centerX = m_mainLayer->getContentSize().width / 2.f;
    auto menu = CCMenu::create();
    menu->setPosition({ 0.f, 0.f });
    m_tabContent->addChild(menu);

    auto createRow = [&](const char* labelText, TextInput*& inputRef, float y) {
        auto lbl = geode::Label::create(labelText, "bigFont.fnt");
        lbl->setAnchorPoint({ 1.f, 0.5f });
        lbl->setLimitLabelWidth(138.f, 0.33f, 0.2f);
        lbl->setPosition({ 166.f, y });
        m_tabContent->addChild(lbl);

        inputRef = TextInput::create(230.f, "0.0", "bigFont.fnt");
        inputRef->setScale(0.62f);
        inputRef->setFilter("0123456789.");
        inputRef->setNumericMode(true);
        inputRef->setPosition({ 296.f, y });
        m_tabContent->addChild(inputRef);
        };

    float startY = 204.f;
    float stepY = 26.f;

    createRow("TPS:", m_tpsInput, startY);
    createRow("Respawn (s):", m_respawnInput, startY - stepY * 1);
    createRow("Target (s):", m_targetTimeInput, startY - stepY * 2);
    createRow("Nerve (K_T):", m_nerveInput, startY - stepY * 3);
    createRow("Fatigue (K_U):", m_fatigueInput, startY - stepY * 4);
    createRow("CPS (K_C):", m_cpsInput, startY - stepY * 5);

    syncCalcInputsFromState();

    auto resetSpr = ButtonSprite::create("Reset Defaults", "goldFont.fnt", "GJ_button_05.png", 0.8f);
    resetSpr->setScale(0.64f);
    auto resetBtn = CCMenuItemSpriteExtra::create(resetSpr, this, menu_selector(SettingsPopup::onResetCalcDefaults));
    resetBtn->setPosition({ centerX, 34.f });
    menu->addChild(resetBtn);
}

void SettingsPopup::syncCalcInputsFromState() {
    if (m_tpsInput)        m_tpsInput->setString(fmt::format("{:.2f}", g_macroFps));
    if (m_respawnInput)    m_respawnInput->setString(fmt::format("{:.2f}", g_respawnTime));
    if (m_targetTimeInput) m_targetTimeInput->setString(fmt::format("{:.1f}", g_targetTime));
    if (m_nerveInput)      m_nerveInput->setString(fmt::format("{:.16f}", g_kT));
    if (m_fatigueInput)    m_fatigueInput->setString(fmt::format("{:.16f}", g_kU));
    if (m_cpsInput)        m_cpsInput->setString(fmt::format("{:.16f}", g_kC));
}

void SettingsPopup::saveCalcInputs() {
    auto parseVal = [](TextInput* input, double defaultVal) -> double {
        if (!input) return defaultVal;
        try {
            std::string str = input->getString();
            if (str.empty()) return defaultVal;
            return std::stod(str);
        }
        catch (...) { return defaultVal; }
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

void SettingsPopup::onResetCalcDefaults(CCObject*) {
    resetCalcSettingsToDefault();
    syncCalcInputsFromState();
    auto alert = FLAlertLayer::create("Settings Reset", "All calculation settings restored to default.", "OK");
    alert->show();
    stopAlertAnimation(alert);
}

// 生命周期与刷新
void SettingsPopup::update(float dt) {
    if (m_currentTab == SettingsTab::Precision && m_calcBtnLabel) {
        if (g_isCalculating.load()) {
            m_calcBtnLabel->setString(fmt::format("Stop ({:.1f}%)", g_calcProgress.load()).c_str());
            m_calcBtnLabel->setColor({ 255, 100, 100 });
        }
        else {
            m_calcBtnLabel->setString("Start Calculation");
            m_calcBtnLabel->setColor({ 255, 255, 255 });
        }
    }
}

void SettingsPopup::onClose(CCObject* sender) {
    if (m_currentTab == SettingsTab::LStarCalc && m_tpsInput) {
        saveCalcInputs();
    }
    Popup::onClose(sender);
}

void SettingsPopup::showInstant() {
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

void SettingsPopup::instantClose() {
    if (m_currentTab == SettingsTab::LStarCalc && m_tpsInput) {
        saveCalcInputs();
    }
    this->removeFromParentAndCleanup(true);
}