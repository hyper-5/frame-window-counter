#pragma once
#include "Types.hpp"
#include <map>
#include <vector>
#include <atomic>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>

extern std::map<std::string, FrameWindowPreset> g_windowPresets;
extern std::map<std::string, LabelPreset> g_labelPresets;
extern std::map<std::string, FrameAction> g_frameActions;
extern std::vector<FrameAction> g_tickActionsCache;
extern std::vector<FrameAction> g_validActions;

extern double g_macroFps;
extern bool g_modEnabled;
extern bool g_showBase, g_showN, g_showF, g_showC, g_showNF, g_showNC, g_showFC, g_showNFC;
extern bool g_forcePrecRedraw;
extern int g_finalLStarDisplayMode;
extern double g_respawnTime;      // 死亡到复活时间 (秒)
extern double g_targetTime;       // 目标求解时间 (秒)
extern double g_kT;               // Nerve 常数
extern double g_kU;               // Fatigue 常数
extern double g_kC;               // CPS 常数

extern std::atomic<int> g_calcId;
extern std::atomic<bool> g_isCalculating;
extern std::atomic<float> g_calcProgress;
extern std::atomic<bool> g_isLStarDirty;
extern std::chrono::steady_clock::time_point g_lastAutoSaveTime;

extern std::vector<double> g_vBase, g_vN, g_vF, g_vC, g_vNF, g_vNC, g_vFC, g_vNFC;

inline std::string formatWindowVal(double val) {
    if (std::abs(val) < 1e-9) return "0";
    std::ostringstream oss;
    oss << std::setprecision(6) << val;
    return oss.str();
}

// 生成二维 window 预设键：格式为 "swift_window"，例如 "1_5" 或 "2_5"
inline std::string makeWindowPresetKey(int swift, double val) {
    return std::to_string(swift) + "_" + formatWindowVal(val);
}

void loadModData();
void saveSettings();
void saveFrames();
void updateTickCache();
void doAutoSave(GJGameLevel* level);
void triggerHUDRefresh();
void resetCalcSettingsToDefault();