#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include "../Data/State.hpp"
#include "../UI/FrameActionPopup.hpp"

using namespace geode::prelude;

class $modify(MyPauseLayer, PauseLayer) {
    static void onModify(auto& self) {
        (void)self.setHookPriorityAfterPost("PauseLayer::customSetup", "geode.node-ids");
    }

    void customSetup() {
        PauseLayer::customSetup();
        loadModData();

        auto spr = CCSprite::createWithSpriteFrameName("GJ_timeIcon_001.png");
        if (!spr) return;
        spr->setScale(0.8f);

        auto btn = CCMenuItemSpriteExtra::create(
            spr,
            this,
            menu_selector(MyPauseLayer::onOpenEditor)
        );
        btn->setID("frame-editor-btn"_spr);

        auto rightMenu = this->getChildByID("right-button-menu");
        if (!rightMenu) {
            rightMenu = this->getChildByIDRecursive("right-button-menu");
        }

        if (rightMenu) {
            rightMenu->addChild(btn);
            rightMenu->updateLayout();
        }
        else {
            auto winSize = CCDirector::sharedDirector()->getWinSize();
            auto fallbackMenu = CCMenu::create();
            fallbackMenu->setID("frame-editor-fallback-menu"_spr);
            fallbackMenu->setPosition({ winSize.width - 36.f, winSize.height / 2.f });
            fallbackMenu->addChild(btn);
            this->addChild(fallbackMenu, 10);
        }
    }

    void onOpenEditor(CCObject*) {
        if (auto popup = FrameActionPopup::create()) {
            popup->setID("FrameActionPopup"_spr);
            popup->showInstant();
        }
    }
};