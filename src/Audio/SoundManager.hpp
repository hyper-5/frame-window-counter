#pragma once
#include <Geode/Geode.hpp>
#include <Geode/fmod/fmod.hpp>
#include <string>
#include <unordered_map>

class SoundManager {
private:
    static std::unordered_map<std::string, FMOD::Sound*> s_soundCache;
    static FMOD::ChannelGroup* s_sfxGroup;

    static bool ensureChannelGroup(FMOD::System* sys);
    //单个音效最大允许并发重叠数
    static constexpr size_t MAX_ACTIVE_VOICES = 6;

public:
    static void playSound(const std::string& path);
    static void stopAll();
    static void clearCache();
};