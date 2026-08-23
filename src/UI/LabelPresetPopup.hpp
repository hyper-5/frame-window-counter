#pragma once
#include <Geode/Geode.hpp>

class LabelPresetPopup : public geode::Popup {
protected:
    geode::TextInput* m_idInput = nullptr;
    geode::TextInput* m_swiftInput = nullptr;
    geode::TextInput* m_minInput = nullptr;
    geode::TextInput* m_maxInput = nullptr;
    geode::TextInput* m_textInput = nullptr;
    geode::TextInput* m_audioInput = nullptr;
    cocos2d::CCSprite* m_colorSprite = nullptr;
    CCMenuItemToggler* m_hudToggle = nullptr;

    cocos2d::ccColor4F m_currentColor = { 1.f, 1.f, 1.f, 1.f };
    bool m_currentShowInHud = false;

    bool init();
    void onLoad(cocos2d::CCObject*);
    void autoSave();
    void onColorBtn(cocos2d::CCObject*);
    void onBrowseAudio(cocos2d::CCObject*);
    void onHudToggle(cocos2d::CCObject*);
    void onApplyColorToWins(cocos2d::CCObject*);
    void onResetAll(cocos2d::CCObject*);
    void onSwitchToFrames(cocos2d::CCObject*);

public:
    static LabelPresetPopup* create();
    void showInstant();
    void instantClose();
};