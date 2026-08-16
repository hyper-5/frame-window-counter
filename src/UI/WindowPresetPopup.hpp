#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>

class WindowPresetPopup : public geode::Popup {
protected:
    geode::TextInput* m_winInput = nullptr;
    cocos2d::CCSprite* m_colorSprite = nullptr;
    cocos2d::ccColor4F m_currentColor = { 1.f, 1.f, 1.f, 1.f };

    bool init();
    void onLoad(cocos2d::CCObject*);
    void onColorBtn(cocos2d::CCObject*);
    void autoSave();
    void onSwitchToFrames(cocos2d::CCObject*);

public:
    static WindowPresetPopup* create();
    void showInstant();
    void instantClose();
};