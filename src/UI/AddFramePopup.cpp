#include "AddFramePopup.hpp"
#include "FrameActionPopup.hpp"
#include "../Data/State.hpp"
#include "../Common.hpp"

using namespace geode::prelude;

bool AddFramePopup::init(FrameActionPopup* parent) {
    if (!Popup::init(220.f, 130.f)) return false;
    m_parentPopup = parent;
    this->setTitle("Add Frame");

    auto size = m_mainLayer->getContentSize();

    m_input = TextInput::create(150.f, "Frame Number");
    m_input->setPosition({ size.width / 2.f, size.height / 2.f });
    m_input->setFilter("0123456789");
    m_mainLayer->addChild(m_input);

    auto btnSpr = ButtonSprite::create("Add");
    btnSpr->setScale(0.8f);
    auto btn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(AddFramePopup::onAdd));
    btn->setPosition({ size.width / 2.f, 25.f });

    auto menu = CCMenu::create();
    menu->setPosition({ 0, 0 });
    menu->addChild(btn);
    m_mainLayer->addChild(menu);

    return true;
}

void AddFramePopup::onAdd(CCObject*) {
    if (!m_input) return;

    std::string text = m_input->getString();
    if (text.empty()) return;
    int frame = 0;
    try { frame = std::stoi(text); }
    catch (...) { return; }

    FrameAction act;
    act.frame = frame;
    act.shouldDraw = true;
    act.frameWindow = 1;
    act.isPlayer2 = false;

    std::string baseStr = std::to_string(frame) + "_0";
    int idx = 0;
    std::string finalKey = baseStr + "_" + std::to_string(idx);

    while (g_frameActions.contains(finalKey)) {
        idx++;
        finalKey = baseStr + "_" + std::to_string(idx);
    }

    g_frameActions[finalKey] = act;
    saveFrames();

    if (m_parentPopup && m_parentPopup->getParent()) {
        if (auto parent = typeinfo_cast<FrameActionPopup*>(m_parentPopup.data())) {
            parent->jumpToFrame(frame);
        }
    }

    this->removeFromParentAndCleanup(true);
}

AddFramePopup* AddFramePopup::create(FrameActionPopup* parent) {
    auto ret = new AddFramePopup();
    if (ret && ret->init(parent)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

void AddFramePopup::showInstant() {
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