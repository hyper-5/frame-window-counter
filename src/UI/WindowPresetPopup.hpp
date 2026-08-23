#pragma once
#include <Geode/Geode.hpp>

class WindowPresetPopup : public geode::Popup {
protected:
    geode::TextInput* m_swiftInput = nullptr;
    geode::TextInput* m_winInput = nullptr;
    geode::TextInput* m_textInput = nullptr;
    cocos2d::CCSprite* m_colorSprite = nullptr;
    cocos2d::ccColor4F m_currentColor = { 1.f, 1.f, 1.f, 1.f };

    bool init();
    void onLoad(cocos2d::CCObject*);
    void onColorBtn(cocos2d::CCObject*);
    void autoSave();
    void onResetAll(cocos2d::CCObject*);
    void onSwitchToFrames(cocos2d::CCObject*);

public:
    static WindowPresetPopup* create();
    void showInstant();
    void instantClose();
};