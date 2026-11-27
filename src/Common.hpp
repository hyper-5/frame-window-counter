#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

// 割线法最大迭代次数
#define MAXITERATION1 10
// 二分法最大迭代次数
#define MAXITERATION2 60
// 搜索区间最大右边界值
#define MAXRIGHT 1000000000000000.0
// 搜索区间最小左边界值
#define MINLEFT 0.001
// 自动保存时间间隔 (秒)
#define AUTOSAVETIME 180

// 默认模型与计算常数
constexpr double DEFAULT_MACRO_FPS = 240.0;        // 默认 TPS
constexpr double DEFAULT_RESPAWN_TIME = 0.0;       // 默认复活时间 (秒)
constexpr double DEFAULT_TARGET_TIME = 86400.0;    // 默认目标求解时间 24小时 (秒)
constexpr double DEFAULT_K_T = 0.0016520833717346; // Nerve
constexpr double DEFAULT_K_U = 0.0002727763242154; // Fatigue
constexpr double DEFAULT_K_C = 0.2784421686721826; // CPS

// 默认显示外观常数
constexpr float DEFAULT_MARKER_CIRCLE_SCALE = 1.0f; // 圆圈缩放
constexpr float DEFAULT_MARKER_TEXT_SCALE = 0.5f; // 圆圈左侧数字缩放
constexpr float DEFAULT_HUD_TEXT_SCALE = 0.5f; // 左上角计数文字缩放
constexpr float DEFAULT_HUD_LINE_SPACING = 18.0f;// 左上角计数行间距
constexpr float DEFAULT_PREC_TEXT_SCALE = 0.3f; // 左下角 Precision 文字缩放
constexpr float DEFAULT_PREC_LINE_SPACING = 12.0f; // 左下角 Precision 行间距

constexpr bool DEFAULT_SHOW_INPUT_TIMELINE = true; // 默认开启实时输入时间轴图表
constexpr bool DEFAULT_SHOW_HUD_LABELS = true; // 默认显示左上角计数标签
constexpr bool DEFAULT_AUTO_SAVE_ENABLED = true; // 默认开启自动保存
constexpr int DEFAULT_AUTO_SAVE_INTERVAL = AUTOSAVETIME; // 默认自动保存间隔 (秒)
constexpr bool DEFAULT_PLAY_AUDIO = true; // 默认开启音频播放

inline void stopAlertAnimation(FLAlertLayer* alert) {
    if (!alert) return;
    if (alert->m_mainLayer) {
        alert->m_mainLayer->stopAllActions();
        alert->m_mainLayer->setScale(1.0f);
    }
}

inline void closeAllModPopups() {
#if defined(GEODE_IS_MOBILE)
    auto scene = cocos2d::CCDirector::sharedDirector()->getRunningScene();
    if (!scene) return;

    auto children = scene->getChildren();
    if (!children) return;

    std::string modID = Mod::get()->getID();

    for (int i = children->count() - 1; i >= 0; i--) {
        auto child = static_cast<cocos2d::CCNode*>(children->objectAtIndex(i));
        if (!child) continue;

        // 1. 判断是否是由 Popup 派生的弹窗
        bool isPopup = (typeinfo_cast<Popup*>(child) != nullptr);

        // 2. 判断节点的 ID 是否以本 Mod ID 开头
        std::string childID = child->getID();
        bool isModNode = !childID.empty() && (childID.rfind(modID, 0) == 0);

        if (isPopup || isModNode) {
            child->removeFromParentAndCleanup(true);
        }
    }
#endif
}