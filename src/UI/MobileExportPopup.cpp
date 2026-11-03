#include "MobileExportPopup.hpp"
#include "../IO/FileIO.hpp"
#include "../Common.hpp"
#include <chrono>
#include <ctime>
#include <algorithm>

using namespace geode::prelude;

bool MobileExportPopup::init() {
    if (!Popup::init(340.f, 210.f)) return false;
    this->setTitle("Export Macro");

    auto size = m_mainLayer->getContentSize();
    float centerX = size.width / 2.f;

    auto menu = CCMenu::create();
    menu->setPosition({ 0.f, 0.f });
    m_mainLayer->addChild(menu);

    auto nameLbl = CCLabelBMFont::create("File Name:", "bigFont.fnt");
    nameLbl->setScale(0.45f);
    nameLbl->setPosition({ centerX, 150.f });
    m_mainLayer->addChild(nameLbl);

    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(GEODE_IS_WINDOWS)
    localtime_s(&tm, &timeT);
#else
    localtime_r(&timeT, &tm);
#endif
    char timeBuf[32];
    std::strftime(timeBuf, sizeof(timeBuf), "export_%Y%m%d_%H%M%S", std::localtime(&timeT));

    m_nameInput = TextInput::create(240.f, timeBuf);
    m_nameInput->setString(timeBuf);
    m_nameInput->setPosition({ centerX, 115.f });
    m_nameInput->setFilter("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-. ");
    m_mainLayer->addChild(m_nameInput);

    auto fwcSpr = ButtonSprite::create("Export .FWC");
    fwcSpr->setScale(0.65f);
    auto fwcBtn = CCMenuItemSpriteExtra::create(fwcSpr, this, menu_selector(MobileExportPopup::onExportFWC));
    fwcBtn->setPosition({ centerX - 70.f, 50.f });
    menu->addChild(fwcBtn);

    auto jsonSpr = ButtonSprite::create("Export .JSON");
    jsonSpr->setScale(0.65f);
    auto jsonBtn = CCMenuItemSpriteExtra::create(jsonSpr, this, menu_selector(MobileExportPopup::onExportJSON));
    jsonBtn->setPosition({ centerX + 70.f, 50.f });
    menu->addChild(jsonBtn);

    return true;
}

void MobileExportPopup::onExportFWC(CCObject*) {
    this->doExport(false);
}

void MobileExportPopup::onExportJSON(CCObject*) {
    this->doExport(true);
}

void MobileExportPopup::doExport(bool isJson) {
    std::string baseName = m_nameInput ? m_nameInput->getString() : "";
    if (baseName.empty()) {
        baseName = "export_output";
    }

    std::string ext = isJson ? ".json" : ".fwc";
    if (baseName.length() >= ext.length()) {
        auto sub = baseName.substr(baseName.length() - ext.length());
        std::transform(sub.begin(), sub.end(), sub.begin(), ::tolower);
        if (sub != ext) {
            baseName += ext;
        }
    }
    else {
        baseName += ext;
    }

    auto exportDir = Mod::get()->getSaveDir() / "exports";
    std::error_code ec;
    std::filesystem::create_directories(exportDir, ec);

    auto targetPath = exportDir / baseName;
    this->removeFromParentAndCleanup(true);

    FileIO::exportToFile(targetPath, isJson);
}

MobileExportPopup* MobileExportPopup::create() {
    auto ret = new MobileExportPopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

void MobileExportPopup::showInstant() {
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