#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/Label.hpp>
#include <Geode/ui/TextInput.hpp>

enum class SettingsTab {
    Display = 0,
    Labels,
    Windows,
    Precision,
    LStarCalc
};

class SettingsPopup : public geode::Popup {
protected:
    SettingsTab m_currentTab = SettingsTab::Display;
    static SettingsTab s_lastTab;

    cocos2d::CCMenu* m_tabMenu = nullptr;
    cocos2d::CCNode* m_tabContent = nullptr;
    bool m_suppressCallbacks = false;

    // --- Display Tab ---
    Slider* m_circleScaleSlider = nullptr;
    geode::TextInput* m_circleScaleInput = nullptr;

    Slider* m_markerTextSlider = nullptr;
    geode::TextInput* m_markerTextInput = nullptr;

    Slider* m_hudTextSlider = nullptr;
    geode::TextInput* m_hudTextInput = nullptr;

    Slider* m_hudSpacingSlider = nullptr;
    geode::TextInput* m_hudSpacingInput = nullptr;

    Slider* m_precTextSlider = nullptr;
    geode::TextInput* m_precTextInput = nullptr;

    Slider* m_precSpacingSlider = nullptr;
    geode::TextInput* m_precSpacingInput = nullptr;

    CCMenuItemToggler* m_timelineToggle = nullptr;

    CCMenuItemToggler* m_hudLabelsToggle = nullptr;

    CCMenuItemToggler* m_autoSaveToggle = nullptr;
    geode::TextInput* m_autoSaveIntervalInput = nullptr;

    CCMenuItemToggler* m_playAudioToggle = nullptr;

    // --- Labels Tab ---
    geode::TextInput* m_labelIdInput = nullptr;
    geode::TextInput* m_labelMinInput = nullptr;
    geode::TextInput* m_labelMaxInput = nullptr;
    geode::TextInput* m_labelTextInput = nullptr;
    geode::TextInput* m_labelAudioInput = nullptr;
    geode::Label* m_labelMinLbl = nullptr;
    geode::Label* m_labelMaxLbl = nullptr;
    CCMenuItemToggler* m_labelIfToggle = nullptr;
    CCMenuItemToggler* m_labelHudToggle = nullptr;
    cocos2d::CCSprite* m_labelColorSprite = nullptr;
    cocos2d::ccColor4F m_labelCurrentColor = { 1.f, 1.f, 1.f, 1.f };
    bool m_labelCurrentUseIF = false;
    bool m_labelCurrentShowInHud = false;
    std::string m_labelMinWinStr;
    std::string m_labelMaxWinStr;
    std::string m_labelMinIFStr;
    std::string m_labelMaxIFStr;
    int m_lastLabelId = 0;

    // --- Windows Tab ---
    geode::TextInput* m_winIfInput = nullptr;
    geode::TextInput* m_winValInput = nullptr;
    geode::TextInput* m_winTextInput = nullptr;
    cocos2d::CCSprite* m_winColorSprite = nullptr;
    cocos2d::ccColor4F m_winCurrentColor = { 1.f, 1.f, 1.f, 1.f };
    int m_lastWinIF = 1;
    std::string m_lastWinStr = "1";

    // --- Precision Tab ---
    geode::Label* m_calcBtnLabel = nullptr;

    // --- L* Calc Tab ---
    geode::TextInput* m_tpsInput = nullptr;
    geode::TextInput* m_respawnInput = nullptr;
    geode::TextInput* m_targetTimeInput = nullptr;
    geode::TextInput* m_nerveInput = nullptr;
    geode::TextInput* m_fatigueInput = nullptr;
    geode::TextInput* m_cpsInput = nullptr;

    bool init(SettingsTab initialTab);
    void clearTabPointers();
    void rebuildTabBar();
    void switchTab(SettingsTab tab);
    void onTabClicked(cocos2d::CCObject* sender);

    // 各 Tab 构建函数
    void buildDisplayTab();
    void buildLabelsTab();
    void buildWindowsTab();
    void buildPrecisionTab();
    void buildCalcTab();

    // Display Tab 回调
    void onDisplaySliderChanged(cocos2d::CCObject* sender);
    void onResetVisuals(cocos2d::CCObject* sender);
    void onToggleInputTimeline(CCObject* sender);
    void onToggleHudLabels(cocos2d::CCObject* sender);
    void onToggleAutoSave(cocos2d::CCObject* sender);
    void onTogglePlayAudio(cocos2d::CCObject* sender);

    // Labels Tab 回调
    void loadLabelPreset();
    void saveLabelPreset();
    void onLabelPrevId(cocos2d::CCObject* sender);
    void onLabelNextId(cocos2d::CCObject* sender);
    void onLabelIFToggle(cocos2d::CCObject* sender);
    void onLabelHudToggle(cocos2d::CCObject* sender);
    void onLabelColorBtn(cocos2d::CCObject* sender);
    void onLabelBrowseAudio(cocos2d::CCObject* sender);
    void onLabelApplyToWins(cocos2d::CCObject* sender);
    void onLabelResetAll(cocos2d::CCObject* sender);

    // Windows Tab 回调
    void loadWindowPreset();
    void saveWindowPreset();
    void onWinPrevIF(cocos2d::CCObject* sender);
    void onWinNextIF(cocos2d::CCObject* sender);
    void onWinColorBtn(cocos2d::CCObject* sender);
    void onWinResetAll(cocos2d::CCObject* sender);

    // Precision Tab 回调
    void onTBase(cocos2d::CCObject*);
    void onTN(cocos2d::CCObject*);
    void onTF(cocos2d::CCObject*);
    void onTC(cocos2d::CCObject*);
    void onTNF(cocos2d::CCObject*);
    void onTNC(cocos2d::CCObject*);
    void onTFC(cocos2d::CCObject*);
    void onTNFC(cocos2d::CCObject*);
    void onToggleCalc(cocos2d::CCObject*);

    // L* Calc Tab 回调
    void syncCalcInputsFromState();
    void saveCalcInputs();
    void onResetCalcDefaults(cocos2d::CCObject*);

    void update(float dt) override;

public:
    static SettingsPopup* create();
    static SettingsPopup* create(SettingsTab initialTab);
    void onClose(cocos2d::CCObject* sender) override;
    void showInstant();
    void instantClose();
};