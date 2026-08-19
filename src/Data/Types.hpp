#pragma once
#include <Geode/Geode.hpp>
#include <matjson/std.hpp>
#include <gdr/gdr.hpp>
#include <string>
#include <cmath>

struct MyReplay : gdr::Replay<MyReplay, gdr::Input<>> {
    MyReplay() : gdr::Replay<MyReplay, gdr::Input<>>("FrameCounter", 1) {}
};

struct FrameWindowPreset {
    double window = 1.0;
    cocos2d::ccColor4F color = { 1.f, 1.f, 1.f, 1.f };
};

template <>
struct matjson::Serialize<FrameWindowPreset> {
    static geode::Result<FrameWindowPreset> fromJson(matjson::Value const& value) {
        if (!value.isObject()) return geode::Err("Expected object");
        FrameWindowPreset p;
        p.window = value["window"].asDouble().unwrapOr(1.0);
        p.color.r = static_cast<float>(value["r"].asDouble().unwrapOr(1.0));
        p.color.g = static_cast<float>(value["g"].asDouble().unwrapOr(1.0));
        p.color.b = static_cast<float>(value["b"].asDouble().unwrapOr(1.0));
        p.color.a = static_cast<float>(value["a"].asDouble().unwrapOr(1.0));
        return geode::Ok(p);
    }
    static matjson::Value toJson(FrameWindowPreset const& p) {
        return matjson::makeObject({
            {"window", p.window},
            {"r", static_cast<double>(p.color.r)},
            {"g", static_cast<double>(p.color.g)},
            {"b", static_cast<double>(p.color.b)},
            {"a", static_cast<double>(p.color.a)}
            });
    }
};

struct LabelPreset {
    int id = 1;
    std::string minWindowStr = "";
    std::string maxWindowStr = "";
    std::string text = "Label";
    std::string audioPath = "";
    cocos2d::ccColor4F color = { 1.f, 1.f, 1.f, 1.f };
    bool showInHud = true;
    double minVal = 0.0;
    double maxVal = 999999.0;

    void updateBounds() {
        try { minVal = minWindowStr.empty() ? 0.0 : std::stod(minWindowStr); }
        catch (...) { minVal = 0.0; }
        try { maxVal = maxWindowStr.empty() ? 999999.0 : std::stod(maxWindowStr); }
        catch (...) { maxVal = 999999.0; }
    }
};

template <>
struct matjson::Serialize<LabelPreset> {
    static geode::Result<LabelPreset> fromJson(matjson::Value const& value) {
        if (!value.isObject()) return geode::Err("Expected object");
        LabelPreset p;
        p.id = value["id"].asInt().unwrapOr(1);
        p.minWindowStr = value["minW"].asString().unwrapOr("");
        p.maxWindowStr = value["maxW"].asString().unwrapOr("");
        p.text = value["text"].asString().unwrapOr("1");
        p.audioPath = value["audioPath"].asString().unwrapOr("");
        p.color.r = static_cast<float>(value["r"].asDouble().unwrapOr(1.0));
        p.color.g = static_cast<float>(value["g"].asDouble().unwrapOr(1.0));
        p.color.b = static_cast<float>(value["b"].asDouble().unwrapOr(1.0));
        p.color.a = static_cast<float>(value["a"].asDouble().unwrapOr(1.0));
        p.showInHud = value["showInHud"].asBool().unwrapOr(false);
        p.updateBounds();
        return geode::Ok(p);
    }
    static matjson::Value toJson(LabelPreset const& p) {
        return matjson::makeObject({
            {"id", p.id},
            {"minW", p.minWindowStr},
            {"maxW", p.maxWindowStr},
            {"text", p.text},
            {"audioPath", p.audioPath},
            {"r", static_cast<double>(p.color.r)},
            {"g", static_cast<double>(p.color.g)},
            {"b", static_cast<double>(p.color.b)},
            {"a", static_cast<double>(p.color.a)},
            {"showInHud", p.showInHud}
            });
    }
};

struct FrameAction {
    int frame = 0;
    bool shouldDraw = true;
    double frameWindow = 1.0;
    bool isPlayer2 = false;
};