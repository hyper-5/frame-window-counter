#include "LabelPresetPopup.hpp"
#include "FrameActionPopup.hpp"
#include "../Data/State.hpp"
#include "../Common.hpp"
#if defined(GEODE_IS_ANDROID) || defined(GEODE_IS_IOS)
#include "MobileAudioPickerPopup.hpp"
#endif
#include <Geode/ui/ColorPickPopup.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/async.hpp>
#include <set>
#include <optional>

using namespace geode::prelude;

bool LabelPresetPopup::init() {
    if (!Popup::init(400.f, 280.f)) return false;
    this->setTitle("Label Interval Settings");

    auto size = m_mainLayer->getContentSize();
    float centerX = size.width / 2;

    auto menu = CCMenu::create();
    menu->setPosition({ 0, 0 });
    m_mainLayer->addChild(menu);

    // ----------------- 第一行：ID -----------------
    auto idLbl = CCLabelBMFont::create("ID:", "bigFont.fnt");
    idLbl->setScale(0.45f);
    idLbl->setPosition({ centerX - 45.f, 235.f });
    m_mainLayer->addChild(idLbl);

    m_idInput = TextInput::create(70.f, "0");
    m_idInput->setPosition({ centerX + 15.f, 235.f });
    m_idInput->setFilter("0123456789");
    m_idInput->setString("0");
    m_idInput->setCallback([this](std::string const&) {
        this->onLoad(nullptr);
        });
    m_mainLayer->addChild(m_idInput);

    // ----------------- 第二行：I/F 切换按钮、Min 与 Max -----------------
    auto ifLbl = CCLabelBMFont::create("I/F:", "bigFont.fnt");
    ifLbl->setScale(0.38f);
    ifLbl->setPosition({ centerX - 145.f, 195.f });
    m_mainLayer->addChild(ifLbl);

    m_ifToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(LabelPresetPopup::onIFToggle), 0.65f);
    m_ifToggle->setPosition({ centerX - 105.f, 195.f });
    menu->addChild(m_ifToggle);

    m_minLbl = CCLabelBMFont::create("Min Win:", "bigFont.fnt");
    m_minLbl->setScale(0.38f);
    m_minLbl->setPosition({ centerX - 55.f, 195.f });
    m_mainLayer->addChild(m_minLbl);

    m_minInput = TextInput::create(50.f, "0");
    m_minInput->setPosition({ centerX - 5.f, 195.f });
    m_minInput->setFilter("0123456789./");
    m_minInput->setCallback([this](std::string const& text) {
        if (m_currentUseIF) m_currentMinIFStr = text;
        else m_currentMinWindowStr = text;
        this->autoSave();
        });
    m_mainLayer->addChild(m_minInput);

    m_maxLbl = CCLabelBMFont::create("Max Win:", "bigFont.fnt");
    m_maxLbl->setScale(0.38f);
    m_maxLbl->setPosition({ centerX + 55.f, 195.f });
    m_mainLayer->addChild(m_maxLbl);

    m_maxInput = TextInput::create(60.f, "999999");
    m_maxInput->setPosition({ centerX + 115.f, 195.f });
    m_maxInput->setFilter("0123456789./");
    m_maxInput->setCallback([this](std::string const& text) {
        if (m_currentUseIF) m_currentMaxIFStr = text;
        else m_currentMaxWindowStr = text;
        this->autoSave();
        });
    m_mainLayer->addChild(m_maxInput);

    // ----------------- 第三行至底部：原有设置 -----------------
    m_textInput = TextInput::create(260.f, "HUD Display Text");
    m_textInput->setPosition({ centerX, 155.f });
    m_textInput->setFilter("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");
    m_textInput->setCallback([this](std::string const&) { this->autoSave(); });
    m_mainLayer->addChild(m_textInput);

    m_audioInput = TextInput::create(190.f, "Audio Path");
    m_audioInput->setPosition({ centerX - 50.f, 115.f });
    m_audioInput->setFilter("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");
    m_audioInput->setCallback([this](std::string const&) { this->autoSave(); });
    m_mainLayer->addChild(m_audioInput);

    auto browseSpr = ButtonSprite::create("Browse");
    browseSpr->setScale(0.7f);
    auto browseBtn = CCMenuItemSpriteExtra::create(browseSpr, this, menu_selector(LabelPresetPopup::onBrowseAudio));
    browseBtn->setPosition({ centerX + 110.f, 115.f });
    menu->addChild(browseBtn);

    auto colorLabel = CCLabelBMFont::create("HUD Color:", "bigFont.fnt");
    colorLabel->setScale(0.5f);
    colorLabel->setPosition({ centerX - 110.f, 75.f });
    m_mainLayer->addChild(colorLabel);

    m_colorSprite = CCSprite::createWithSpriteFrameName("GJ_colorBtn_001.png");
    m_colorSprite->setColor({
        static_cast<GLubyte>(m_currentColor.r * 255),
        static_cast<GLubyte>(m_currentColor.g * 255),
        static_cast<GLubyte>(m_currentColor.b * 255)
        });
    m_colorSprite->setOpacity(static_cast<GLubyte>(m_currentColor.a * 255));

    auto colorWrapper = CCNode::create();
    colorWrapper->setContentSize(m_colorSprite->getContentSize());
    m_colorSprite->setPosition(colorWrapper->getContentSize() / 2);
    colorWrapper->addChild(m_colorSprite);
    colorWrapper->setScale(0.65f);

    auto colorBtn = CCMenuItemSpriteExtra::create(colorWrapper, this, menu_selector(LabelPresetPopup::onColorBtn));
    colorBtn->setPosition({ centerX - 50.f, 75.f });
    menu->addChild(colorBtn);

    auto hudLabel = CCLabelBMFont::create("Show in HUD:", "bigFont.fnt");
    hudLabel->setScale(0.5f);
    hudLabel->setPosition({ centerX + 40.f, 75.f });
    m_mainLayer->addChild(hudLabel);

    m_hudToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(LabelPresetPopup::onHudToggle), 0.7f);
    m_hudToggle->setPosition({ centerX + 120.f, 75.f });
    menu->addChild(m_hudToggle);

    auto switchBtnSpr = ButtonSprite::create("<- Back");
    switchBtnSpr->setScale(0.6f);
    auto switchBtn = CCMenuItemSpriteExtra::create(switchBtnSpr, this, menu_selector(LabelPresetPopup::onSwitchToFrames));
    switchBtn->setPosition({ centerX - 110.f, 30.f });
    menu->addChild(switchBtn);

    auto applyBtnSpr = ButtonSprite::create("Apply to Wins");
    applyBtnSpr->setScale(0.6f);
    auto applyBtn = CCMenuItemSpriteExtra::create(applyBtnSpr, this, menu_selector(LabelPresetPopup::onApplyColorToWins));
    applyBtn->setPosition({ centerX, 30.f });
    menu->addChild(applyBtn);

    auto resetBtnSpr = ButtonSprite::create("Reset All");
    resetBtnSpr->setScale(0.6f);
    auto resetBtn = CCMenuItemSpriteExtra::create(resetBtnSpr, this, menu_selector(LabelPresetPopup::onResetAll));
    resetBtn->setPosition({ centerX + 110.f, 30.f });
    menu->addChild(resetBtn);

    this->onLoad(nullptr);
    return true;
}

void LabelPresetPopup::onIFToggle(CCObject* sender) {
    if (auto toggle = typeinfo_cast<CCMenuItemToggler*>(sender)) {
        if (m_currentUseIF) {
            m_currentMinIFStr = m_minInput->getString();
            m_currentMaxIFStr = m_maxInput->getString();
        }
        else {
            m_currentMinWindowStr = m_minInput->getString();
            m_currentMaxWindowStr = m_maxInput->getString();
        }

        m_currentUseIF = !toggle->isToggled();

        if (m_currentUseIF) {
            if (m_minLbl) m_minLbl->setString("Min I/F:");
            if (m_maxLbl) m_maxLbl->setString("Max I/F:");
            m_minInput->setString(m_currentMinIFStr.empty() ? "1" : m_currentMinIFStr);
            m_maxInput->setString(m_currentMaxIFStr.empty() ? "999999" : m_currentMaxIFStr);
        }
        else {
            if (m_minLbl) m_minLbl->setString("Min Win:");
            if (m_maxLbl) m_maxLbl->setString("Max Win:");
            m_minInput->setString(m_currentMinWindowStr.empty() ? "0" : m_currentMinWindowStr);
            m_maxInput->setString(m_currentMaxWindowStr.empty() ? "999999" : m_currentMaxWindowStr);
        }

        this->autoSave();
    }
}

void LabelPresetPopup::onApplyColorToWins(CCObject*) {
    std::string minStr = m_minInput->getString();
    std::string maxStr = m_maxInput->getString();

    int count = 0;

    if (m_currentUseIF) {
        int minIF = static_cast<int>(std::round(parseWindowExpr(minStr, 1.0)));
        int maxIF = static_cast<int>(std::round(parseWindowExpr(maxStr, 999999.0)));

        if (minIF < 1) minIF = 1;
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
                preset.color = m_currentColor;
                count++;
            }
        }

        std::set<double> existingWindows;
        for (auto const& [key, preset] : g_windowPresets) {
            existingWindows.insert(preset.window);
        }
        if (existingWindows.empty()) {
            existingWindows.insert(1.0);
        }

        for (int k = minIF; k <= maxIF; k++) {
            for (double w : existingWindows) {
                auto key = makeWindowPresetKey(k, w);
                if (!g_windowPresets.contains(key)) {
                    FrameWindowPreset p;
                    p.ifCount = k;
                    p.window = w;
                    p.color = m_currentColor;
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
        double minWin = parseWindowExpr(minStr, 0.0);
        double maxWin = parseWindowExpr(maxStr, 999999.0);

        if (minWin < 0.0) minWin = 0.0;
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
                preset.color = m_currentColor;
                count++;
            }
        }

        std::set<double> targetWindows;
        targetWindows.insert(minWin);
        targetWindows.insert(maxWin);

        int startInt = static_cast<int>(std::ceil(minWin - 1e-6));
        int endInt = static_cast<int>(std::floor(maxWin + 1e-6));
        for (int i = startInt; i <= endInt; ++i) {
            targetWindows.insert(static_cast<double>(i));
        }

        std::set<int> existingIFs;
        for (auto const& [key, preset] : g_windowPresets) {
            existingIFs.insert(preset.ifCount);
        }
        existingIFs.insert(1);

        for (int k : existingIFs) {
            for (double w : targetWindows) {
                if (w < 0.0) continue;
                auto key = makeWindowPresetKey(k, w);
                if (!g_windowPresets.contains(key)) {
                    FrameWindowPreset p;
                    p.ifCount = k;
                    p.window = w;
                    p.color = m_currentColor;
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

void LabelPresetPopup::onBrowseAudio(CCObject*) {
#if defined(GEODE_IS_ANDROID) || defined(GEODE_IS_IOS)
    Ref<LabelPresetPopup> safeThis = this;
    auto popup = MobileAudioPickerPopup::create([safeThis](std::string const& selectedPath) {
        if (safeThis && safeThis->getParent() && safeThis->m_audioInput) {
            safeThis->m_audioInput->setString(selectedPath);
            safeThis->autoSave();
        }
        });
    if (popup) {
        popup->showInstant();
    }
#else
    file::FilePickOptions::Filter filter = {
        "Audio Files",
        { "*.ogg", "*.mp3", "*.wav" }
    };
    file::FilePickOptions options;
    options.filters.push_back(filter);

    Ref<LabelPresetPopup> safeThis = this;
    async::spawn(
        file::pick(file::PickMode::OpenFile, options),
        [safeThis](Result<std::optional<asp::fs::Path>> result) {
            if (!result.isOk()) return;
            auto opt = result.unwrap();
            if (!opt.has_value()) return;

            if (safeThis && safeThis->getParent() && safeThis->m_audioInput) {
                safeThis->m_audioInput->setString(opt.value().string());
                safeThis->autoSave();
            }
        }
    );
#endif
}

void LabelPresetPopup::onColorBtn(CCObject*) {
    Ref<LabelPresetPopup> safeThis = this;

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

void LabelPresetPopup::onHudToggle(CCObject* sender) {
    if (auto toggle = typeinfo_cast<CCMenuItemToggler*>(sender)) {
        m_currentShowInHud = !toggle->isToggled();
        this->autoSave();
    }
}

void LabelPresetPopup::autoSave() {
    std::string idStr = m_idInput ? m_idInput->getString() : "";
    if (idStr.empty()) return;

    LabelPreset p;
    try { p.id = std::stoi(idStr); }
    catch (...) { p.id = 0; }

    p.useIF = m_currentUseIF;
    p.minWindowStr = m_currentMinWindowStr;
    p.maxWindowStr = m_currentMaxWindowStr;
    p.minIFStr = m_currentMinIFStr;
    p.maxIFStr = m_currentMaxIFStr;
    p.text = m_textInput ? m_textInput->getString() : "";
    p.audioPath = m_audioInput ? m_audioInput->getString() : "";
    p.color = m_currentColor;
    p.showInHud = m_currentShowInHud;
    p.updateBounds();

    g_labelPresets[idStr] = p;
    saveSettings();

    triggerHUDRefresh();
}

void LabelPresetPopup::onLoad(CCObject*) {
    std::string idStr = m_idInput ? m_idInput->getString() : "0";
    if (idStr.empty()) return;

    if (g_labelPresets.contains(idStr)) {
        auto& p = g_labelPresets[idStr];
        m_currentUseIF = p.useIF;
        if (m_ifToggle) m_ifToggle->toggle(m_currentUseIF);

        m_currentMinWindowStr = p.minWindowStr;
        m_currentMaxWindowStr = p.maxWindowStr;
        m_currentMinIFStr = p.minIFStr;
        m_currentMaxIFStr = p.maxIFStr;

        if (m_currentUseIF) {
            if (m_minLbl) m_minLbl->setString("Min I/F:");
            if (m_maxLbl) m_maxLbl->setString("Max I/F:");
            if (m_minInput) m_minInput->setString(p.minIFStr);
            if (m_maxInput) m_maxInput->setString(p.maxIFStr);
        }
        else {
            if (m_minLbl) m_minLbl->setString("Min Win:");
            if (m_maxLbl) m_maxLbl->setString("Max Win:");
            if (m_minInput) m_minInput->setString(p.minWindowStr);
            if (m_maxInput) m_maxInput->setString(p.maxWindowStr);
        }

        if (m_textInput) m_textInput->setString(p.text);
        if (m_audioInput) m_audioInput->setString(p.audioPath);

        m_currentColor = p.color;
        if (m_colorSprite) {
            m_colorSprite->setColor({
                static_cast<GLubyte>(p.color.r * 255),
                static_cast<GLubyte>(p.color.g * 255),
                static_cast<GLubyte>(p.color.b * 255)
                });
            m_colorSprite->setOpacity(static_cast<GLubyte>(p.color.a * 255));
        }

        m_currentShowInHud = p.showInHud;
        if (m_hudToggle) m_hudToggle->toggle(m_currentShowInHud);
    }
    else {
        m_currentUseIF = false;
        if (m_ifToggle) m_ifToggle->toggle(false);

        m_currentMinWindowStr = idStr;
        m_currentMaxWindowStr = idStr;
        m_currentMinIFStr = "";
        m_currentMaxIFStr = "";

        if (m_minLbl) m_minLbl->setString("Min Win:");
        if (m_maxLbl) m_maxLbl->setString("Max Win:");
        if (m_minInput) m_minInput->setString(idStr);
        if (m_maxInput) m_maxInput->setString(idStr);

        if (m_textInput) m_textInput->setString(idStr);
        if (m_audioInput) m_audioInput->setString("");

        m_currentColor = { 1.f, 1.f, 1.f, 1.f };
        if (m_colorSprite) {
            m_colorSprite->setColor({ 255, 255, 255 });
            m_colorSprite->setOpacity(255);
        }

        m_currentShowInHud = false;
        if (m_hudToggle) m_hudToggle->toggle(false);
    }
}

void LabelPresetPopup::onResetAll(CCObject*) {
    Ref<LabelPresetPopup> safeThis = this;

    auto alert = geode::createQuickPopup(
        "Reset All Labels",
        "Are you sure you want to clear <cr>ALL label presets</c>?",
        "Cancel", "Reset",
        [safeThis](auto, bool btn2) {
            if (btn2) {
                g_labelPresets.clear();
                saveSettings();
                triggerHUDRefresh();

                if (safeThis && safeThis->getParent()) {
                    safeThis->onLoad(nullptr);
                }

                auto successAlert = FLAlertLayer::create("Success", "All label presets have been cleared.", "OK");
                successAlert->show();
                stopAlertAnimation(successAlert);
            }
        }
    );
    stopAlertAnimation(alert);
}

void LabelPresetPopup::onSwitchToFrames(CCObject*) {
    this->removeFromParentAndCleanup(true);
    auto popup = FrameActionPopup::create();
    if (popup) {
        popup->setID("FrameActionPopup"_spr);
        popup->showInstant();
    }
}

LabelPresetPopup* LabelPresetPopup::create() {
    auto ret = new LabelPresetPopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

void LabelPresetPopup::showInstant() {
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

void LabelPresetPopup::instantClose() {
    this->removeFromParentAndCleanup(true);
}