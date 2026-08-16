#pragma once
#include <Geode/Geode.hpp>

//割线法最大迭代次数
#define MAXITERATION1 10
//二分法最大迭代次数
#define MAXITERATION2 60
//搜索区间最大右边界值
#define MAXRIGHT 1000000000000000.0
//搜索区间最小左边界值
#define MINLEFT 0.001
//自动保存时间间隔 (秒)
#define AUTOSAVETIME 180

//默认模型与计算常数
constexpr double DEFAULT_MACRO_FPS = 240.0;        // 默认 TPS
constexpr double DEFAULT_RESPAWN_TIME = 0.0;       // 默认复活时间 (秒)
constexpr double DEFAULT_TARGET_TIME = 86400.0;    // 默认目标求解时间 24小时 (秒)
constexpr double DEFAULT_K_T = 0.0016520833717346; // Nerve
constexpr double DEFAULT_K_U = 0.0002727763242154; // Fatigue
constexpr double DEFAULT_K_C = 0.2784421686721826; // CPS

inline void stopAlertAnimation(FLAlertLayer* alert) {
    if (!alert) return;
    if (alert->m_mainLayer) {
        alert->m_mainLayer->stopAllActions();
        alert->m_mainLayer->setScale(1.0f);
    }
}