#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>
#include "../Data/State.hpp"
#include "../Common.hpp"
#include "../UI/AddFramePopup.hpp"
#include "../UI/FrameActionPopup.hpp"
#include "../UI/PrecisionSettingsPopup.hpp"
#include "../UI/LStarCalcSettingsPopup.hpp"
#include "../UI/LabelPresetPopup.hpp"
#include "../UI/WindowPresetPopup.hpp"

#ifdef GEODE_IS_WINDOWS
#include <windows.h>
#endif

using namespace geode::prelude;

// 递归检测当前场景树中是否有任何文本输入框处于聚焦输入状态
static bool isAnyTextInputFocused(CCNode* root) {
    if (!root) return false;

    // CCTextInputNode 的选中焦点变量名为 m_selected
    if (auto textInput = typeinfo_cast<CCTextInputNode*>(root)) {
        if (textInput->m_selected) {
            return true;
        }
    }

    if (auto children = root->getChildren()) {
        for (int i = 0; i < children->count(); i++) {
            auto child = static_cast<CCNode*>(children->objectAtIndex(i));
            if (isAnyTextInputFocused(child)) {
                return true;
            }
        }
    }
    return false;
}

class $modify(MyDirector, CCDirector) {
    void drawScene() {
        CCDirector::drawScene();

        auto scene = this->getRunningScene();
        if (!scene || typeinfo_cast<CCTransitionScene*>(scene)) {
            return;
        }

        // 自动保存逻辑
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - g_lastAutoSaveTime).count() >= AUTOSAVETIME) {
            g_lastAutoSaveTime = now;
            if (auto pl = PlayLayer::get(); pl && pl->m_level) {
                doAutoSave(pl->m_level);
            }
        }

        // Windows 全局快捷键监听
#ifdef GEODE_IS_WINDOWS
        if (GetActiveWindow() != nullptr) {
            static bool s_hotkeyPressed = false;
            if (GetAsyncKeyState('O') & 0x8000) {
                if (!s_hotkeyPressed) {
                    s_hotkeyPressed = true;

                    // 如果用户当前正在任何输入框内打字，忽略快捷键触发
                    if (!isAnyTextInputFocused(scene)) {
                        bool closedAny = false;
                        if (auto children = scene->getChildren()) {
                            for (int i = children->count() - 1; i >= 0; i--) {
                                auto child = static_cast<CCNode*>(children->objectAtIndex(i));

                                // 关闭所有关联的弹窗
                                if (typeinfo_cast<AddFramePopup*>(child) ||
                                    typeinfo_cast<FrameActionPopup*>(child) ||
                                    typeinfo_cast<PrecisionSettingsPopup*>(child) ||
                                    typeinfo_cast<LStarCalcSettingsPopup*>(child) ||
                                    typeinfo_cast<LabelPresetPopup*>(child) ||
                                    typeinfo_cast<WindowPresetPopup*>(child)) {
                                    child->removeFromParentAndCleanup(true);
                                    closedAny = true;
                                }
                            }
                        }

                        // 若未打开任何弹窗，则弹出帧数编辑器
                        if (!closedAny) {
                            if (auto popup = FrameActionPopup::create()) {
                                popup->setID("FrameActionPopup"_spr);
                                popup->showInstant();
                            }
                        }
                    }
                }
            }
            else {
                s_hotkeyPressed = false;
            }
        }
#endif

        // 关卡运行中的实时帧高亮追踪
        if (g_modEnabled) {
            if (auto playLayer = PlayLayer::get()) {
                if (!playLayer->m_isPaused && playLayer->m_player1 && !playLayer->m_player1->m_isDead) {
                    if (auto popup = typeinfo_cast<FrameActionPopup*>(scene->getChildByID("FrameActionPopup"_spr))) {
                        int currentFrame = static_cast<int>(playLayer->m_gameState.m_levelTime * g_macroFps);
                        popup->doTrackingTick(currentFrame);
                    }
                }
            }
        }
    }
};