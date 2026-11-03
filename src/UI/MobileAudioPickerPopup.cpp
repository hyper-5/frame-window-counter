#include "MobileAudioPickerPopup.hpp"
#include "../Common.hpp"
#include <asp/fs/std.hpp>
#include <algorithm>

using namespace geode::prelude;

bool MobileAudioPickerPopup::init(std::function<void(std::string)> callback) {
    if (!Popup::init(380.f, 250.f)) return false;
    this->setTitle("Select Audio File");
    m_callback = callback;

    auto size = m_mainLayer->getContentSize();
    float centerX = size.width / 2.f;

    auto topMenu = CCMenu::create();
    topMenu->setPosition({ 0.f, 0.f });
    m_mainLayer->addChild(topMenu);

    auto infoSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    infoSpr->setScale(0.6f);
    auto infoBtn = CCMenuItemSpriteExtra::create(infoSpr, this, menu_selector(MobileAudioPickerPopup::onShowPath));
    infoBtn->setPosition({ size.width - 25.f, size.height - 25.f });
    topMenu->addChild(infoBtn);

    auto refreshSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    refreshSpr->setScale(0.6f);
    auto refreshBtn = CCMenuItemSpriteExtra::create(refreshSpr, this, menu_selector(MobileAudioPickerPopup::onRefresh));
    refreshBtn->setPosition({ size.width - 55.f, size.height - 25.f });
    topMenu->addChild(refreshBtn);

    // 居中滚动画布
    m_scrollLayer = ScrollLayer::create({ 340.f, 160.f });
    m_scrollLayer->setPosition({ (size.width - 340.f) / 2.f, 30.f });
    m_mainLayer->addChild(m_scrollLayer);

    m_emptyLabel = CCLabelBMFont::create("No audio files found!\nPlace .ogg, .mp3 or .wav\ninto mod 'audio' folder.", "bigFont.fnt");
    m_emptyLabel->setScale(0.35f);
    m_emptyLabel->setAlignment(kCCTextAlignmentCenter);
    m_emptyLabel->setPosition({ centerX, 110.f });
    m_emptyLabel->setColor({ 200, 200, 200 });
    m_emptyLabel->setVisible(false);
    m_mainLayer->addChild(m_emptyLabel);

    this->scanFiles();
    this->buildList();

    return true;
}

void MobileAudioPickerPopup::scanFiles() {
    m_fileList.clear();
    std::filesystem::path dir = (Mod::get()->getSaveDir() / "audio").toStdPath().unwrapOrDefault();
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);

    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".ogg" || ext == ".mp3" || ext == ".wav") {
                m_fileList.push_back(entry.path());
            }
        }
    }

    std::sort(m_fileList.begin(), m_fileList.end(), [](const auto& a, const auto& b) {
        std::string nameA = a.filename().string();
        std::string nameB = b.filename().string();
        std::transform(nameA.begin(), nameA.end(), nameA.begin(), ::tolower);
        std::transform(nameB.begin(), nameB.end(), nameB.begin(), ::tolower);
        return nameA < nameB;
        });
}

void MobileAudioPickerPopup::buildList() {
    if (!m_scrollLayer || !m_scrollLayer->m_contentLayer) return;
    m_scrollLayer->m_contentLayer->removeAllChildrenWithCleanup(true);

    if (m_fileList.empty()) {
        if (m_emptyLabel) m_emptyLabel->setVisible(true);
        return;
    }
    if (m_emptyLabel) m_emptyLabel->setVisible(false);

    float rowHeight = 36.f;
    float topPadding = 4.f;
    float bottomPadding = 4.f;
    float contentHeight = std::max(160.f, topPadding + bottomPadding + static_cast<float>(m_fileList.size()) * rowHeight);

    m_scrollLayer->m_contentLayer->setContentSize({ 340.f, contentHeight });

    float currentY = contentHeight - topPadding - (rowHeight / 2.f);

    for (size_t i = 0; i < m_fileList.size(); ++i) {
        const auto& path = m_fileList[i];

        auto cell = CCNode::create();
        cell->setContentSize({ 330.f, 32.f });
        cell->setAnchorPoint({ 0.5f, 0.5f });
        cell->ignoreAnchorPointForPosition(false);
        cell->setPosition({ 170.f, currentY });

        // 背景框
        auto bg = CCScale9Sprite::create("square02_small.png");
        bg->setContentSize({ 330.f, 32.f });
        bg->setOpacity(80);
        bg->setPosition({ 165.f, 16.f });
        cell->addChild(bg);

        // 按钮交互菜单
        auto rowMenu = CCMenu::create();
        rowMenu->setPosition({ 0.f, 0.f });
        rowMenu->setContentSize({ 330.f, 32.f });
        cell->addChild(rowMenu);

        // 音频标记
        auto audioTag = CCLabelBMFont::create("[AUDIO]", "chatFont.fnt");
        audioTag->setScale(0.48f);
        audioTag->setAnchorPoint({ 0.f, 0.5f });
        audioTag->setPosition({ 8.f, 16.f });
        audioTag->setColor({ 255, 130, 220 });
        cell->addChild(audioTag);

        // 音频文件名
        std::string fileName = path.filename().string();
        if (fileName.length() > 22) {
            fileName = fileName.substr(0, 19) + "...";
        }
        auto nameLabel = CCLabelBMFont::create(fileName.c_str(), "chatFont.fnt");
        nameLabel->setScale(0.55f);

        // 文件名支持直接点击触发
        auto nameBtn = CCMenuItemSpriteExtra::create(nameLabel, this, menu_selector(MobileAudioPickerPopup::onSelectFile));
        nameBtn->setUserObject(cocos2d::CCString::create(path.string()));
        nameBtn->setAnchorPoint({ 0.f, 0.5f });
        nameBtn->setPosition({ 60.f, 16.f });
        rowMenu->addChild(nameBtn);

        // 右侧 Pick 按钮
        auto selectSpr = ButtonSprite::create("Pick");
        selectSpr->setScale(0.5f);
        auto selectBtn = CCMenuItemSpriteExtra::create(selectSpr, this, menu_selector(MobileAudioPickerPopup::onSelectFile));
        selectBtn->setUserObject(cocos2d::CCString::create(path.string()));
        selectBtn->setPosition({ 290.f, 16.f });
        rowMenu->addChild(selectBtn);

        m_scrollLayer->m_contentLayer->addChild(cell);
        currentY -= rowHeight;
    }

    m_scrollLayer->moveToTop();
}

void MobileAudioPickerPopup::onSelectFile(CCObject* sender) {
    auto btn = typeinfo_cast<CCMenuItemSpriteExtra*>(sender);
    if (!btn) return;
    auto strObj = static_cast<cocos2d::CCString*>(btn->getUserObject());
    if (!strObj) return;

    std::string selectedPath = strObj->getCString();
    if (m_callback) {
        m_callback(selectedPath);
    }
    this->removeFromParentAndCleanup(true);
}

void MobileAudioPickerPopup::onRefresh(CCObject*) {
    this->scanFiles();
    this->buildList();
}

void MobileAudioPickerPopup::onShowPath(CCObject*) {
    auto dir = Mod::get()->getSaveDir() / "audio";
    auto alert = FLAlertLayer::create(
        "Audio Directory",
        fmt::format("Place audio files in:\n<cg>{}</c>", dir.string()).c_str(),
        "OK"
    );
    alert->show();
    stopAlertAnimation(alert);
}

MobileAudioPickerPopup* MobileAudioPickerPopup::create(std::function<void(std::string)> callback) {
    auto ret = new MobileAudioPickerPopup();
    if (ret && ret->init(callback)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

void MobileAudioPickerPopup::showInstant() {
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