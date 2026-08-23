#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

class PrecisionSettingsPopup : public geode::Popup {
protected:
    cocos2d::CCLabelBMFont* m_calcBtnLabel = nullptr;

    bool init() override;
    void onTBase(cocos2d::CCObject*);
    void onTN(cocos2d::CCObject*);
    void onTF(cocos2d::CCObject*);
    void onTC(cocos2d::CCObject*);
    void onTNF(cocos2d::CCObject*);
    void onTNC(cocos2d::CCObject*);
    void onTFC(cocos2d::CCObject*);
    void onTNFC(cocos2d::CCObject*);
    void onToggleCalc(cocos2d::CCObject*);
    void update(float dt) override;
    void onOpenCalcSettings(cocos2d::CCObject*);

public:
    static PrecisionSettingsPopup* create();
    void showInstant();
};