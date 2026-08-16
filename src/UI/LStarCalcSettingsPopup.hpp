#pragma once
#include <Geode/Geode.hpp>

class LStarCalcSettingsPopup : public geode::Popup {
protected:
    geode::TextInput* m_tpsInput = nullptr;
    geode::TextInput* m_respawnInput = nullptr;
    geode::TextInput* m_targetTimeInput = nullptr;
    geode::TextInput* m_nerveInput = nullptr;
    geode::TextInput* m_fatigueInput = nullptr;
    geode::TextInput* m_cpsInput = nullptr;

    bool init();
    void saveInputs();
    void syncInputsFromState();

    void onResetDefaults(cocos2d::CCObject*);
    void onClose(cocos2d::CCObject*);

public:
    static LStarCalcSettingsPopup* create();
};