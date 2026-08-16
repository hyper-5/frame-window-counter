#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include "../Data/State.hpp"
#include "../UI/FrameActionPopup.hpp"

using namespace geode::prelude;

class $modify(MyPauseLayer, PauseLayer) {

    //暂停界面初始化与自定义按钮构建
    void customSetup() {
        PauseLayer::customSetup();
        loadModData();
        auto rightMenu = this->getChildByID("right-button-menu");
        if (rightMenu) {
            auto btn = CCMenuItemSpriteExtra::create(
                CCSprite::createWithSpriteFrameName("GJ_timeIcon_001.png"),
                this,
                menu_selector(MyPauseLayer::onOpenEditor)
            );
            btn->setScale(0.8f);
            btn->setID("frame-editor-btn"_spr);
            rightMenu->addChild(btn);
            rightMenu->updateLayout();
        }
    }

    //点击暂停界面按钮打开帧数编辑窗口
    void onOpenEditor(CCObject*) {
        if (auto popup = FrameActionPopup::create()) {
            popup->setID("FrameActionPopup"_spr);
            popup->showInstant();
        }
    }
};