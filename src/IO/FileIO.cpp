#include "FileIO.hpp"
#include "CmlParser.hpp"
#include "../Data/State.hpp"
#include "../Common.hpp"
#if defined(GEODE_IS_MOBILE)
#include "../UI/MobileExportPopup.hpp"
#include "../UI/MobileImportPopup.hpp"
#endif
#include <slc/slc.hpp>
#include <gdr/gdr.hpp>
#include <gdr_convert.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/async.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <optional>
#include <span>

using namespace geode::prelude;

namespace FileIO {

    static void doExportToFile(const std::filesystem::path& path, const std::vector<FrameAction>& exportList, bool isJson) {
        std::ofstream f(path.string(), isJson ? std::ios::out : std::ios::binary);
        if (!f) {
            geode::queueInMainThread([]() {
                auto alert = FLAlertLayer::create("Error", "Failed to open file for writing.", "OK");
                alert->show(); stopAlertAnimation(alert);
                });
            return;
        }

        if (isJson) {
            std::vector<matjson::Value> arr;
            int inputCounter = 1;

            for (const auto& act : exportList) {
                if (!act.shouldDraw) continue;

                double base_frame = static_cast<double>(act.frame);
                int k = act.ifCount > 0 ? act.ifCount : 1;
                double fw = (act.frameWindow <= 0.0) ? 1.0 : act.frameWindow;

                if (k <= 1) {
                    arr.push_back(matjson::makeObject({
                        {"input", inputCounter++},
                        {"timePosition", act.frame},
                        {"frameWindow", fw},
                        {"isPlayer2", act.isPlayer2}
                        }));
                }
                else {
                    double w_main = std::max(fw - (k - 1.0) / k, 1.0 / k);
                    double w_sub = 1.0 / k;

                    arr.push_back(matjson::makeObject({
                        {"input", inputCounter++},
                        {"timePosition", base_frame},
                        {"frameWindow", w_main},
                        {"isPlayer2", act.isPlayer2}
                        }));

                    for (int s = 1; s < k; ++s) {
                        double sub_frame = base_frame + static_cast<double>(s) / static_cast<double>(k);
                        arr.push_back(matjson::makeObject({
                            {"input", inputCounter++},
                            {"timePosition", sub_frame},
                            {"frameWindow", w_sub},
                            {"isPlayer2", act.isPlayer2}
                            }));
                    }
                }
            }

            matjson::Value rootObject = matjson::makeObject({
                {"format", "nandl-calculator"},
                {"version", 1},
                {"settings", matjson::makeObject({
                    {"gameFps", g_macroFps},
                    {"windowFps", g_macroFps},
                    {"respawnSeconds", 0},
                    {"timeUnit", "frames"}
                })},
                {"frameWindows", arr}
                });
            f << rootObject.dump(2);
        }
        else {
            f.write("FWC2", 4);
            double fps = g_macroFps;
            f.write(reinterpret_cast<const char*>(&fps), sizeof(double));
            uint32_t count = static_cast<uint32_t>(exportList.size());
            f.write(reinterpret_cast<const char*>(&count), sizeof(uint32_t));

            for (const auto& act : exportList) {
                int32_t frame = act.frame;
                double frameWindow = act.frameWindow;
                uint8_t flags = (act.shouldDraw ? 1 : 0) | (act.isPlayer2 ? 2 : 0);
                int32_t ifCount = act.ifCount;

                f.write(reinterpret_cast<const char*>(&frame), sizeof(int32_t));
                f.write(reinterpret_cast<const char*>(&frameWindow), sizeof(double));
                f.write(reinterpret_cast<const char*>(&flags), sizeof(uint8_t));
                f.write(reinterpret_cast<const char*>(&ifCount), sizeof(int32_t));
            }
        }
        f.close();

        std::string fileName = path.filename().string();
        geode::queueInMainThread([fileName]() {
            auto alert = FLAlertLayer::create("Success", fmt::format("Successfully exported file:\n{}", fileName).c_str(), "OK");
            alert->show(); stopAlertAnimation(alert);
            });
    }

    static void doImportFromFile(const std::filesystem::path& path, std::function<void()> onSuccessCallback) {
        try {
            std::vector<FrameAction> newActions;
            double parsedFps = 240.0;
            bool updateFps = false;
            std::string ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".fwc") {
                std::ifstream f(path.string(), std::ios::binary);
                if (!f) throw std::runtime_error("Cannot open .fwc file");
                char magic[5] = { 0 };
                f.read(magic, 4);
                std::string magicStr(magic);

                bool isLegacy = (magicStr == "FWCB");
                bool isNew = (magicStr == "FWC2");
                if (!isLegacy && !isNew) throw std::runtime_error("Invalid binary FWC file signature");

                f.read(reinterpret_cast<char*>(&parsedFps), sizeof(double));
                updateFps = true;
                uint32_t count = 0;
                f.read(reinterpret_cast<char*>(&count), sizeof(uint32_t));

                for (uint32_t i = 0; i < count; ++i) {
                    int32_t frame = 0;
                    double frameWindow = 1.0;
                    uint8_t flags = 0;
                    int32_t ifCount = 1;

                    f.read(reinterpret_cast<char*>(&frame), sizeof(int32_t));
                    if (isLegacy) {
                        int32_t legacyWindow = 1;
                        f.read(reinterpret_cast<char*>(&legacyWindow), sizeof(int32_t));
                        frameWindow = static_cast<double>(legacyWindow);
                    }
                    else {
                        f.read(reinterpret_cast<char*>(&frameWindow), sizeof(double));
                    }
                    f.read(reinterpret_cast<char*>(&flags), sizeof(uint8_t));

                    if (!isLegacy) {
                        f.read(reinterpret_cast<char*>(&ifCount), sizeof(int32_t));
                        if (ifCount < 1) ifCount = 1;
                    }

                    newActions.push_back({ frame, (flags & 1) != 0, frameWindow, (flags & 2) != 0, ifCount });
                }
            }
            else if (ext == ".json") {
                std::ifstream f(path.string());
                if (!f) throw std::runtime_error("Cannot open .json file");
                std::stringstream buffer; buffer << f.rdbuf();
                auto res = matjson::parse(buffer.str());
                if (res.isOk()) {
                    auto root = res.unwrap();
                    if (root.contains("settings")) {
                        parsedFps = root["settings"]["gameFps"].asDouble().unwrapOr(240.0);
                        updateFps = true;
                    }
                    if (root.contains("frameWindows") && root["frameWindows"].isArray()) {
                        for (auto& item : root["frameWindows"].asArray().unwrap()) {
                            FrameAction act;
                            double timePos = item["timePosition"].asDouble().unwrapOr(0.0);
                            act.frame = static_cast<int>(std::floor(timePos));
                            act.shouldDraw = true;
                            act.frameWindow = item["frameWindow"].asDouble().unwrapOr(1.0);

                            bool isP2 = false;
                            if (item.contains("isPlayer2")) {
                                isP2 = item["isPlayer2"].asBool().unwrapOr(false);
                            }
                            act.isPlayer2 = isP2;

                            int ifVal = 1;
                            if (item.contains("if")) {
                                ifVal = item["if"].asInt().unwrapOr(1);
                            }
                            else if (item.contains("swift")) {
                                int oldSwift = item["swift"].asInt().unwrapOr(0);
                                ifVal = oldSwift <= 0 ? 1 : (oldSwift + 1);
                            }
                            if (ifVal < 1) ifVal = 1;
                            act.ifCount = ifVal;

                            newActions.push_back(act);
                        }
                    }
                }
            }
            else if (ext == ".slc") {
                std::ifstream file(path.string(), std::ios::binary);
                if (!file) throw std::runtime_error("Cannot open .slc file");
                auto readRes = slc::Replay<>::read(file);
                if (!readRes) throw std::runtime_error("Failed to parse slc file");
                auto replay = readRes.value();
                parsedFps = replay.m_meta.m_tps;
                updateFps = true;

                for (const auto& atomVariant : replay.m_atoms.m_atoms) {
                    if (const auto* actionAtom = std::get_if<slc::ActionAtom>(&atomVariant)) {
                        for (const auto& action : actionAtom->m_actions) {
                            newActions.push_back({ static_cast<int>(action.m_frame), false, 1.0, action.m_player2, 1 });
                        }
                    }
                }
            }
            else if (ext == ".gdr2" || ext == ".gdr") {
                std::vector<gdr::Input<>> inputs;
                if (ext == ".gdr2") {
                    auto importRes = MyReplay::importData(path.string());
                    if (importRes.isOk()) {
                        inputs = importRes.unwrap().inputs;
                        parsedFps = importRes.unwrap().framerate;
                        updateFps = true;
                    }
                    else throw std::runtime_error("Failed to parse .gdr2 file");
                }
                else {
                    std::ifstream f(path.string(), std::ios::binary);
                    if (!f) throw std::runtime_error("Cannot open .gdr file");
                    f.seekg(0, std::ios::end);
                    size_t size = f.tellg();
                    f.seekg(0, std::ios::beg);
                    std::vector<uint8_t> data(size);
                    f.read(reinterpret_cast<char*>(data.data()), size);
                    auto convertRes = gdr::convert<MyReplay, gdr::Input<>>(std::span<uint8_t>(data));
                    if (convertRes.isOk()) {
                        inputs = convertRes.unwrap().inputs;
                        parsedFps = convertRes.unwrap().framerate;
                        updateFps = true;
                    }
                    else throw std::runtime_error("Failed to convert .gdr file");
                }
                std::sort(inputs.begin(), inputs.end(), [](const auto& a, const auto& b) { return a.frame < b.frame; });
                for (const auto& input : inputs) {
                    newActions.push_back({ static_cast<int>(input.frame), false, 1.0, input.player2, 1 });
                }
            }
            else if (ext == ".cml") {
                auto res = CmlParser::parse(path);
                newActions = std::move(res.actions);
                if (res.hasFps && res.fps > 0.0) {
                    parsedFps = res.fps;
                    updateFps = true;
                }
            }

            geode::queueInMainThread([newActions, parsedFps, updateFps, onSuccessCallback]() {
                if (newActions.empty()) {
                    auto alert = FLAlertLayer::create("Error", "No valid frames found or empty file.", "OK");
                    alert->show(); stopAlertAnimation(alert);
                    return;
                }
                if (updateFps && parsedFps > 0.0) {
                    g_macroFps = parsedFps;
                    saveSettings();
                }

                g_frameActions.clear();
                int addedCount = 0;
                for (auto& act : newActions) {
                    std::string baseStr = std::to_string(act.frame) + (act.isPlayer2 ? "_1" : "_0");
                    int idx = 0;
                    std::string finalKey = baseStr + "_" + std::to_string(idx);
                    while (g_frameActions.contains(finalKey)) {
                        idx++;
                        finalKey = baseStr + "_" + std::to_string(idx);
                    }
                    g_frameActions[finalKey] = act;
                    addedCount++;
                }

                saveFrames();
                auto alert = FLAlertLayer::create("Success", fmt::format("Loaded {} operations.", addedCount).c_str(), "OK");
                alert->show(); stopAlertAnimation(alert);
                if (onSuccessCallback) onSuccessCallback();
                });
        }
        catch (const std::exception& e) {
            std::string errMsg = e.what();
            geode::queueInMainThread([errMsg]() {
                auto alert = FLAlertLayer::create("Error", fmt::format("Failed: {}", errMsg).c_str(), "OK");
                alert->show(); stopAlertAnimation(alert);
                });
        }
    }

    void exportToFile(const std::filesystem::path& path, bool isJson) {
        std::vector<FrameAction> exportList;
        for (auto& [k, v] : g_frameActions) exportList.push_back(v);

        std::stable_sort(exportList.begin(), exportList.end(), [](const FrameAction& a, const FrameAction& b) {
            return a.frame < b.frame;
            });

        async::runtime().spawnBlocking<void>([path, exportList, isJson]() {
            doExportToFile(path, exportList, isJson);
            });
    }

    void importFromFile(const std::filesystem::path& path, std::function<void()> onSuccessCallback) {
        async::runtime().spawnBlocking<void>([path, onSuccessCallback]() {
            doImportFromFile(path, onSuccessCallback);
            });
    }

    void exportFWC() {
#if defined(GEODE_IS_MOBILE)
        // 移动端
        if (auto popup = MobileExportPopup::create()) {
            popup->showInstant();
        }
#else
        // PC
        std::vector<FrameAction> exportList;
        for (auto& [k, v] : g_frameActions) exportList.push_back(v);

        std::stable_sort(exportList.begin(), exportList.end(), [](const FrameAction& a, const FrameAction& b) {
            return a.frame < b.frame;
            });

        file::FilePickOptions options;
        options.defaultPath = "frames.fwc";
        options.filters.push_back({ "Frame Window Counter (*.fwc)", { "*.fwc" } });
        options.filters.push_back({ "NANDL Calculator JSON (*.json)", { "*.json" } });

        async::spawn(
            file::pick(file::PickMode::SaveFile, options),
            [exportList](Result<std::optional<std::filesystem::path>> pickResult) {
                if (!pickResult.isOk()) return;
                auto optPath = pickResult.unwrap();
                if (!optPath.has_value()) return;

                auto path = optPath.value();
                std::string ext = path.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                bool isJson = (ext == ".json");

                async::runtime().spawnBlocking<void>([path, exportList, isJson]() {
                    doExportToFile(path, exportList, isJson);
                    });
            }
        );
#endif
    }

    void importReplay(std::function<void()> onSuccessCallback) {
        loadModData();

#if defined(GEODE_IS_MOBILE)
        // 移动端
        if (auto popup = MobileImportPopup::create(onSuccessCallback)) {
            popup->showInstant();
        }
#else
        // PC端
        file::FilePickOptions options;
        options.filters.push_back({
            "Supported Formats",
            { "*.fwc", "*.json", "*.gdr", "*.gdr2", "*.slc", "*.cml" }
            });
        options.filters.push_back({ "Frame Window Counter (*.fwc)", { "*.fwc" } });
        options.filters.push_back({ "NANDL Calculator JSON (*.json)", { "*.json" } });
        options.filters.push_back({ "GD Replay / Silicate (*.gdr, *.gdr2, *.slc)", { "*.gdr", "*.gdr2", "*.slc" } });
        options.filters.push_back({ "xdBot Macro (*.cml)", { "*.cml" } });

        async::spawn(
            file::pick(file::PickMode::OpenFile, options),
            [onSuccessCallback](Result<std::optional<std::filesystem::path>> pickResult) {
                if (!pickResult.isOk()) return;
                auto optPath = pickResult.unwrap();
                if (!optPath.has_value()) return;

                auto path = optPath.value();
                async::runtime().spawnBlocking<void>([path, onSuccessCallback]() {
                    doImportFromFile(path, onSuccessCallback);
                    });
            }
        );
#endif
    }
}