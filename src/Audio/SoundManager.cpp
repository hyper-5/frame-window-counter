#include "SoundManager.hpp"
#include <filesystem>
#include <vector>

using namespace geode::prelude;

std::unordered_map<std::string, FMOD::Sound*> SoundManager::s_soundCache;
static std::unordered_map<std::string, std::vector<FMOD::Channel*>> s_activeChannels;
FMOD::ChannelGroup* SoundManager::s_sfxGroup = nullptr;

bool SoundManager::ensureChannelGroup(FMOD::System* sys) {
    if (!sys) return false;
    if (s_sfxGroup) return true;

    //创建专属于 Mod 的音效通道组
    FMOD_RESULT res = sys->createChannelGroup("ModFrameSFXGroup", &s_sfxGroup);
    return (res == FMOD_OK && s_sfxGroup != nullptr);
}

void SoundManager::playSound(const std::string& path) {
    if (path.empty()) return;

    auto audioEngine = FMODAudioEngine::sharedEngine();
    if (!audioEngine || !audioEngine->m_system) return;

    //游戏静音时直接跳过
    if (audioEngine->m_sfxVolume <= 0.0f) return;

    FMOD::System* sys = audioEngine->m_system;
    if (!ensureChannelGroup(sys)) return;

    //实时同步游戏设置里的 SFX 音量
    s_sfxGroup->setVolume(audioEngine->m_sfxVolume);

    //解析音频路径
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

        //1.清理该音效已播放完毕的失效通道
        auto& channels = s_activeChannels[fullPathStr];
        for (auto chIt = channels.begin(); chIt != channels.end(); ) {
            bool isPlaying = false;
            if (*chIt && (*chIt)->isPlaying(&isPlaying) == FMOD_OK && isPlaying) {
                ++chIt;
            }
            else {
                chIt = channels.erase(chIt);
            }
        }

        //2.超出发音上限时停止最旧的一个通道
        while (channels.size() >= MAX_ACTIVE_VOICES) {
            if (channels.front()) {
                channels.front()->stop();
            }
            channels.erase(channels.begin());
        }

        //3.播放新音效
        FMOD::Channel* channel = nullptr;
        FMOD_RESULT playRes = sys->playSound(sound, s_sfxGroup, false, &channel);

        if (playRes == FMOD_OK && channel) {
            //强制播放通道为非循环模式
            channel->setMode(FMOD_LOOP_OFF);
            channel->setLoopCount(0);
            //设置为最高优先级 0
            channel->setPriority(0);
            //禁用音量平滑爬升
            channel->setVolumeRamp(false);
            channels.push_back(channel);
        }
    }
}

void SoundManager::stopAll() {
    if (s_sfxGroup) {
        s_sfxGroup->stop();
    }
    s_activeChannels.clear();
}

void SoundManager::clearCache() {
    stopAll();
    for (auto& [path, sound] : s_soundCache) {
        if (sound) {
            sound->release();
        }
    }
    s_soundCache.clear();
    s_activeChannels.clear();

    if (s_sfxGroup) {
        s_sfxGroup->release();
        s_sfxGroup = nullptr;
    }
}