#include "MobileImportPopup.hpp"
#include "../IO/FileIO.hpp"
#include "../Common.hpp"
#include <asp/fs/std.hpp>
#include <algorithm>

using namespace geode::prelude;

bool MobileImportPopup::init(std::function<void()> onSuccessCallback) {
    if (!Popup::init(380.f, 250.f)) return false;
    this->setTitle("Select Macro to Import");
    m_onSuccessCallback = onSuccessCallback;

    auto size = m_mainLayer->getContentSize();
    float centerX = size.width / 2.f;

    auto topMenu = CCMenu::create();
    topMenu->setPosition({ 0.f, 0.f });
    m_mainLayer->addChild(topMenu);

    // 路径提示按钮
    auto infoSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    infoSpr->setScale(0.6f);
    auto infoBtn = CCMenuItemSpriteExtra::create(infoSpr, this, menu_selector(MobileImportPopup::onShowPath));
    infoBtn->setPosition({ size.width - 25.f, size.height - 25.f });
    topMenu->addChild(infoBtn);

    // 刷新按钮
    auto refreshSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    refreshSpr->setScale(0.6f);
    auto refreshBtn = CCMenuItemSpriteExtra::create(refreshSpr, this, menu_selector(MobileImportPopup::onRefresh));
    refreshBtn->setPosition({ size.width - 55.f, size.height - 25.f });
    topMenu->addChild(refreshBtn);

    // 居中滚动画布
    m_scrollLayer = ScrollLayer::create({ 340.f, 160.f });
    m_scrollLayer->setPosition({ (size.width - 340.f) / 2.f, 30.f });
    m_mainLayer->addChild(m_scrollLayer);

    m_emptyLabel = CCLabelBMFont::create("No macro files found!\nPlace files into 'imports'\nor 'exports' folder.", "bigFont.fnt");
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

void MobileImportPopup::scanFiles() {
    m_fileList.clear();

    std::filesystem::path importDir = (Mod::get()->getSaveDir() / "imports").toStdPath().unwrapOrDefault();
    std::filesystem::path exportDir = (Mod::get()->getSaveDir() / "exports").toStdPath().unwrapOrDefault();
    std::error_code ec;
    std::filesystem::create_directories(importDir, ec);
    std::filesystem::create_directories(exportDir, ec);

    auto isMacroExt = [](const std::string& ext) {
        return ext == ".fwc" || ext == ".json" || ext == ".gdr" || ext == ".gdr2" || ext == ".slc" || ext == ".cml";
        };

    // 1. 扫描 imports 文件夹
    for (const auto& entry : std::filesystem::directory_iterator(importDir, ec)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (isMacroExt(ext)) {
                m_fileList.push_back({ entry.path(), entry.path().filename().string(), false });
            }
        }
    }

    // 2. 扫描 exports 文件夹
    for (const auto& entry : std::filesystem::directory_iterator(exportDir, ec)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (isMacroExt(ext)) {
                m_fileList.push_back({ entry.path(), entry.path().filename().string(), true });
            }
        }
    }

    // 按名称字母排序（不区分大小写）
    std::sort(m_fileList.begin(), m_fileList.end(), [](const MacroFileItem& a, const MacroFileItem& b) {
        std::string nameA = a.filename;
        std::string nameB = b.filename;
        std::transform(nameA.begin(), nameA.end(), nameA.begin(), ::tolower);
        std::transform(nameB.begin(), nameB.end(), nameB.begin(), ::tolower);
        if (nameA != nameB) return nameA < nameB;
        return a.isExport < b.isExport;
        });
}

void MobileImportPopup::buildList() {
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
        const auto& item = m_fileList[i];

        auto cell = CCNode::create();
        cell->setContentSize({ 330.f, 32.f });
        cell->setAnchorPoint({ 0.5f, 0.5f });
        cell->ignoreAnchorPointForPosition(false);
        cell->setPosition({ 170.f, currentY }); // 340 居中

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

        // 来源标签：[IMP] 或 [EXP]
        auto tagLabel = CCLabelBMFont::create(item.isExport ? "[EXP]" : "[IMP]", "chatFont.fnt");
        tagLabel->setScale(0.48f);
        tagLabel->setAnchorPoint({ 0.f, 0.5f });
        tagLabel->setPosition({ 8.f, 16.f });
        tagLabel->setColor(item.isExport ? ccColor3B{ 80, 220, 255 } : ccColor3B{ 255, 205, 80 });
        cell->addChild(tagLabel);

        // 文件名显示
        std::string displayName = item.filename;
        if (displayName.length() > 22) {
            displayName = displayName.substr(0, 19) + "...";
        }
        auto nameLabel = CCLabelBMFont::create(displayName.c_str(), "chatFont.fnt");
        nameLabel->setScale(0.55f);

        // 将文件名也包装为可点击项
        auto nameBtn = CCMenuItemSpriteExtra::create(nameLabel, this, menu_selector(MobileImportPopup::onSelectFile));
        nameBtn->setUserObject(cocos2d::CCString::create(item.path.string()));
        nameBtn->setAnchorPoint({ 0.f, 0.5f });
        nameBtn->setPosition({ 50.f, 16.f });
        rowMenu->addChild(nameBtn);

        // 右侧 Load 按钮
        auto loadSpr = ButtonSprite::create("Load");
        loadSpr->setScale(0.5f);
        auto loadBtn = CCMenuItemSpriteExtra::create(loadSpr, this, menu_selector(MobileImportPopup::onSelectFile));
        loadBtn->setUserObject(cocos2d::CCString::create(item.path.string()));
        loadBtn->setPosition({ 290.f, 16.f });
        rowMenu->addChild(loadBtn);

        m_scrollLayer->m_contentLayer->addChild(cell);
        currentY -= rowHeight;
    }

    m_scrollLayer->moveToTop();
}

void MobileImportPopup::onSelectFile(CCObject* sender) {
    auto btn = typeinfo_cast<CCMenuItemSpriteExtra*>(sender);
    if (!btn) return;
    auto strObj = static_cast<cocos2d::CCString*>(btn->getUserObject());
    if (!strObj) return;

    std::filesystem::path selectedPath = strObj->getCString();
    auto cb = m_onSuccessCallback;
    this->removeFromParentAndCleanup(true);

    FileIO::importFromFile(selectedPath, cb);
}

void MobileImportPopup::onRefresh(CCObject*) {
    this->scanFiles();
    this->buildList();
}

void MobileImportPopup::onShowPath(CCObject*) {
    auto importDir = Mod::get()->getSaveDir() / "imports";
    auto exportDir = Mod::get()->getSaveDir() / "exports";
    auto alert = FLAlertLayer::create(
        "Macro Directories",
        fmt::format("Scan imports:\n<cy>{}</c>\n\nScan exports:\n<cg>{}</c>", importDir.string(), exportDir.string()).c_str(),
        "OK"
    );
    alert->show();
    stopAlertAnimation(alert);
}

MobileImportPopup* MobileImportPopup::create(std::function<void()> onSuccessCallback) {
    auto ret = new MobileImportPopup();
    if (ret && ret->init(onSuccessCallback)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

void MobileImportPopup::showInstant() {
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