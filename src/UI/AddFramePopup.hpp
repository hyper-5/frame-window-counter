#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>

class FrameActionPopup;

class AddFramePopup : public geode::Popup {
protected:
    geode::Ref<cocos2d::CCNode> m_parentPopup = nullptr;
    geode::TextInput* m_input = nullptr;

    bool init(FrameActionPopup* parent);
    void onAdd(cocos2d::CCObject*);

public:
    static AddFramePopup* create(FrameActionPopup* parent);
    void showInstant();
};