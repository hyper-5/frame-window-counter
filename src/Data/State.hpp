#pragma once
#include "Types.hpp"
#include <map>
#include <vector>
#include <atomic>
#include <chrono>

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

void loadModData();
void saveSettings();
void saveFrames();
void updateTickCache();
void doAutoSave(GJGameLevel* level);
void triggerHUDRefresh();
void resetCalcSettingsToDefault();