#include "State.hpp"
#include "../Common.hpp"
#include "../Math/Calculator.hpp"
#include <filesystem>
#include <fstream>
#include <thread>
#include <algorithm>
#include <ctime>

using namespace geode::prelude;

//全局状态与数据缓存定义
std::map<std::string, FrameWindowPreset> g_windowPresets;   // 窗口颜色预设
std::map<std::string, LabelPreset> g_labelPresets;          // 标签区间与音效预设
std::map<std::string, FrameAction> g_frameActions;          // 核心帧动作 Map
std::vector<FrameAction> g_tickActionsCache;                // 按帧排序的快速只读缓存（供 PlayLayer 遍历）
std::vector<FrameAction> g_validActions;                    // 参与 L* 求解的有效操作列表

bool g_modEnabled = true;                                   // Mod 总开关
bool g_showBase = true, g_showN = false, g_showF = false, g_showC = false;
bool g_showNF = false, g_showNC = false, g_showFC = false, g_showNFC = false;
bool g_forcePrecRedraw = true;                              // 强制重绘Precision HUD标记
int g_finalLStarDisplayMode = 0;                            // 编辑器顶部 L* 切换显示模式

//L* 计算参数
double g_macroFps = DEFAULT_MACRO_FPS;
double g_respawnTime = DEFAULT_RESPAWN_TIME;
double g_targetTime = DEFAULT_TARGET_TIME;
double g_kT = DEFAULT_K_T;
double g_kU = DEFAULT_K_U;
double g_kC = DEFAULT_K_C;

//多线程计算同步原子变量
std::atomic<int> g_calcId{ 0 };                             // 当前计算任务唯一ID
std::atomic<bool> g_isCalculating{ false };                 // 是否正在计算中
std::atomic<float> g_calcProgress{ 0.0f };                  // 总体百分比计算进度
std::atomic<bool> g_isLStarDirty{ false };                  // 参数/动作改动标记（用于驱动UI标红）
std::chrono::steady_clock::time_point g_lastAutoSaveTime = std::chrono::steady_clock::now();

//8个不同模式的逐帧精度曲线结果数组
std::vector<double> g_vBase, g_vN, g_vF, g_vC, g_vNF, g_vNC, g_vFC, g_vNFC;

void resetCalcSettingsToDefault() {
    if (g_macroFps != DEFAULT_MACRO_FPS || g_respawnTime != DEFAULT_RESPAWN_TIME ||
        g_targetTime != DEFAULT_TARGET_TIME || g_kT != DEFAULT_K_T ||
        g_kU != DEFAULT_K_U || g_kC != DEFAULT_K_C) {
        g_isLStarDirty = true;
    }
    g_macroFps = DEFAULT_MACRO_FPS;
    g_respawnTime = DEFAULT_RESPAWN_TIME;
    g_targetTime = DEFAULT_TARGET_TIME;
    g_kT = DEFAULT_K_T;
    g_kU = DEFAULT_K_U;
    g_kC = DEFAULT_K_C;
    saveSettings();
}

void loadModData() {
    static bool s_loaded = false;
    if (s_loaded) return;
    s_loaded = true;

    g_windowPresets = Mod::get()->getSavedValue<std::map<std::string, FrameWindowPreset>>("window-presets");
    g_labelPresets = Mod::get()->getSavedValue<std::map<std::string, LabelPreset>>("label-presets-v2");
    g_macroFps = Mod::get()->getSavedValue<double>("macro-fps", DEFAULT_MACRO_FPS);
    g_respawnTime = Mod::get()->getSavedValue<double>("calc-respawn-time", DEFAULT_RESPAWN_TIME);
    g_targetTime = Mod::get()->getSavedValue<double>("calc-target-time", DEFAULT_TARGET_TIME);
    g_kT = Mod::get()->getSavedValue<double>("calc-kt", DEFAULT_K_T);
    g_kU = Mod::get()->getSavedValue<double>("calc-ku", DEFAULT_K_U);
    g_kC = Mod::get()->getSavedValue<double>("calc-kc", DEFAULT_K_C);

    g_modEnabled = Mod::get()->getSavedValue<bool>("mod-enabled", true);
    g_showBase = Mod::get()->getSavedValue<bool>("show-base", true);
    g_showN = Mod::get()->getSavedValue<bool>("show-n", false);
    g_showF = Mod::get()->getSavedValue<bool>("show-f", false);
    g_showC = Mod::get()->getSavedValue<bool>("show-c", false);
    g_showNF = Mod::get()->getSavedValue<bool>("show-nf", false);
    g_showNC = Mod::get()->getSavedValue<bool>("show-nc", false);
    g_showFC = Mod::get()->getSavedValue<bool>("show-fc", false);
    g_showNFC = Mod::get()->getSavedValue<bool>("show-nfc", false);

    for (int i = 0; i <= 99; i++) {
        std::string key = std::to_string(i);
        if (!g_labelPresets.contains(key)) {
            LabelPreset p = { i, "", "", std::to_string(i), "", {1.f, 1.f, 1.f, 1.f}, false };
            p.updateBounds();
            g_labelPresets[key] = p;
        }
    }
}

void saveSettings() {
    Mod::get()->setSavedValue("window-presets", g_windowPresets);
    Mod::get()->setSavedValue("label-presets-v2", g_labelPresets);
    Mod::get()->setSavedValue("macro-fps", g_macroFps);
    Mod::get()->setSavedValue("calc-respawn-time", g_respawnTime);
    Mod::get()->setSavedValue("calc-target-time", g_targetTime);
    Mod::get()->setSavedValue("calc-kt", g_kT);
    Mod::get()->setSavedValue("calc-ku", g_kU);
    Mod::get()->setSavedValue("calc-kc", g_kC);

    Mod::get()->setSavedValue("mod-enabled", g_modEnabled);
    Mod::get()->setSavedValue("show-base", g_showBase);
    Mod::get()->setSavedValue("show-n", g_showN);
    Mod::get()->setSavedValue("show-f", g_showF);
    Mod::get()->setSavedValue("show-c", g_showC);
    Mod::get()->setSavedValue("show-nf", g_showNF);
    Mod::get()->setSavedValue("show-nc", g_showNC);
    Mod::get()->setSavedValue("show-fc", g_showFC);
    Mod::get()->setSavedValue("show-nfc", g_showNFC);
}

void updateTickCache() {
    g_tickActionsCache.clear();
    g_tickActionsCache.reserve(g_frameActions.size());
    for (const auto& [k, v] : g_frameActions) {
        g_tickActionsCache.push_back(v);
    }
    std::stable_sort(g_tickActionsCache.begin(), g_tickActionsCache.end(), [](const FrameAction& a, const FrameAction& b) {
        return a.frame < b.frame;
        });
}

void saveFrames() {
    if (g_isCalculating.load()) {
        stopGlobalRecalc();
    }
    g_isLStarDirty = true;
    updateTickCache();
}

void doAutoSave(GJGameLevel* level) {
    if (g_frameActions.empty()) return;

    std::vector<FrameAction> exportList;
    exportList.reserve(g_frameActions.size());
    for (auto& [k, v] : g_frameActions) exportList.push_back(v);

    std::string levelName = (level && !std::string(level->m_levelName).empty()) ? std::string(level->m_levelName) : "UnknownLevel";
    double fps = g_macroFps;
    auto configDir = Mod::get()->getConfigDir();

    std::thread([exportList = std::move(exportList), levelName, fps, configDir]() mutable {
        std::stable_sort(exportList.begin(), exportList.end(), [](const FrameAction& a, const FrameAction& b) {
            return a.frame < b.frame;
            });

        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::string timeStr = fmt::format("{:04d}-{:02d}-{:02d}_{:02d}-{:02d}-{:02d}",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

        std::string cleanLevelName = "";
        for (char c : levelName) {
            if (std::isalnum(c) || c == ' ' || c == '-' || c == '_') cleanLevelName += c;
        }
        if (cleanLevelName.empty()) cleanLevelName = "Level";

        auto saveDir = configDir / "Autosaves";
        std::filesystem::create_directories(saveDir);
        auto filepath = saveDir / fmt::format("{}_{}.fwc", cleanLevelName, timeStr);

        std::ofstream f(filepath, std::ios::binary);
        if (f) {
            f.write("FWC2", 4);
            f.write(reinterpret_cast<const char*>(&fps), sizeof(double));
            uint32_t count = static_cast<uint32_t>(exportList.size());
            f.write(reinterpret_cast<const char*>(&count), sizeof(uint32_t));

            for (const auto& act : exportList) {
                int32_t frame = act.frame;
                double frameWindow = act.frameWindow;
                uint8_t flags = (act.shouldDraw ? 1 : 0) | (act.isPlayer2 ? 2 : 0);

                f.write(reinterpret_cast<const char*>(&frame), sizeof(int32_t));
                f.write(reinterpret_cast<const char*>(&frameWindow), sizeof(double));
                f.write(reinterpret_cast<const char*>(&flags), sizeof(uint8_t));
            }
            f.close();
            geode::Loader::get()->queueInMainThread([]() {
                geode::Notification::create("Auto-saved FWC", cocos2d::CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png"))->show();
                });
        }
        }).detach();
}

$on_mod(Loaded) {
    loadModData();
}