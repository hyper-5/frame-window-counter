#include "SoundManager.hpp"
#include <filesystem>

using namespace geode::prelude;

std::unordered_map<std::string, FMOD::Sound*> SoundManager::s_soundCache;
FMOD::ChannelGroup* SoundManager::s_sfxGroup = nullptr;

bool SoundManager::ensureChannelGroup(FMOD::System* sys) {
    if (!sys) return false;
    if (s_sfxGroup) return true;

    // 创建专属于 Mod 的音效通道组
    FMOD_RESULT res = sys->createChannelGroup("ModFrameSFXGroup", &s_sfxGroup);
    return (res == FMOD_OK && s_sfxGroup != nullptr);
}

void SoundManager::playSound(const std::string& path) {
    if (path.empty()) return;

    auto audioEngine = FMODAudioEngine::sharedEngine();
    if (!audioEngine || !audioEngine->m_system) return;

    FMOD::System* sys = audioEngine->m_system;
    if (!ensureChannelGroup(sys)) return;

    // 实时同步游戏设置里的 SFX 音量
    s_sfxGroup->setVolume(audioEngine->m_sfxVolume);

    // 解析音频路径
    std::filesystem::path audioPath(path);
    if (!audioPath.is_absolute()) {
        audioPath = Mod::get()->getConfigDir() / path;
    }
    std::string fullPathStr = audioPath.string();

    FMOD::Sound* sound = nullptr;
    auto it = s_soundCache.find(fullPathStr);

    if (it != s_soundCache.end()) {
        sound = it->second;
    }
    else {
        if (!std::filesystem::exists(audioPath)) return;

        FMOD_RESULT res = sys->createSound(
            fullPathStr.c_str(),
            FMOD_DEFAULT | FMOD_LOOP_OFF | FMOD_CREATESAMPLE,
            nullptr,
            &sound
        );

        if (res == FMOD_OK && sound) {
            sound->setMode(FMOD_LOOP_OFF);
            s_soundCache[fullPathStr] = sound;
        }
        else {
            return;
        }
    }

    if (sound) {
        sound->setMode(FMOD_LOOP_OFF);

        FMOD::Channel* channel = nullptr;
        FMOD_RESULT playRes = sys->playSound(sound, s_sfxGroup, false, &channel);

        if (playRes == FMOD_OK && channel) {
            //强制播放通道为非循环模式
            channel->setMode(FMOD_LOOP_OFF);
            channel->setLoopCount(0);

            if (!s_sfxGroup) {
                channel->setVolume(audioEngine->m_sfxVolume);
            }
        }
    }
}

void SoundManager::stopAll() {
    if (s_sfxGroup) {
        s_sfxGroup->stop();
    }
}

void SoundManager::clearCache() {
    stopAll();
    for (auto& [path, sound] : s_soundCache) {
        if (sound) {
            sound->release();
        }
    }
    s_soundCache.clear();

    if (s_sfxGroup) {
        s_sfxGroup->release();
        s_sfxGroup = nullptr;
    }
}