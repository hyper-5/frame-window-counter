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
    int swift = 0;                                          // 所属 Swift 维度
    double window = 1.0;
    cocos2d::ccColor4F color = { 1.f, 1.f, 1.f, 1.f };
    std::string customText = "";
};

// 分式与小数解析函数
inline double parseWindowExpr(const std::string& text, double defaultVal = 1.0) {
    if (text.empty()) return defaultVal;

    auto slashPos = text.find('/');
    if (slashPos != std::string::npos) {
        std::string numStr = text.substr(0, slashPos);
        std::string denStr = text.substr(slashPos + 1);
        if (numStr.empty() || denStr.empty()) return defaultVal;
        if (denStr.find('/') != std::string::npos) return defaultVal; // 防御多个斜杠

        try {
            double num = std::stod(numStr);
            double den = std::stod(denStr);
            if (std::abs(den) < 1e-9) return defaultVal; // 防除以 0
            return num / den;
        }
        catch (...) {
            return defaultVal;
        }
    }

    try {
        return std::stod(text);
    }
    catch (...) {
        return defaultVal;
    }
}

template <>
struct matjson::Serialize<FrameWindowPreset> {
    static geode::Result<FrameWindowPreset> fromJson(matjson::Value const& value) {
        if (!value.isObject()) return geode::Err("Expected object");
        FrameWindowPreset p;
        p.swift = value["swift"].asInt().unwrapOr(0);
        p.window = value["window"].asDouble().unwrapOr(1.0);
        p.color.r = static_cast<float>(value["r"].asDouble().unwrapOr(1.0));
        p.color.g = static_cast<float>(value["g"].asDouble().unwrapOr(1.0));
        p.color.b = static_cast<float>(value["b"].asDouble().unwrapOr(1.0));
        p.color.a = static_cast<float>(value["a"].asDouble().unwrapOr(1.0));
        p.customText = value["customText"].asString().unwrapOr("");
        return geode::Ok(p);
    }
    static matjson::Value toJson(FrameWindowPreset const& p) {
        return matjson::makeObject({
            {"swift", p.swift},
            {"window", p.window},
            {"r", static_cast<double>(p.color.r)},
            {"g", static_cast<double>(p.color.g)},
            {"b", static_cast<double>(p.color.b)},
            {"a", static_cast<double>(p.color.a)},
            {"customText", p.customText}
            });
    }
};

struct LabelPreset {
    int id = 1;
    int swift = 0;                                          // 监听的 Swift 维度
    std::string minWindowStr = "";
    std::string maxWindowStr = "";
    std::string text = "Label";
    std::string audioPath = "";
    cocos2d::ccColor4F color = { 1.f, 1.f, 1.f, 1.f };
    bool showInHud = true;
    double minVal = 0.0;
    double maxVal = 999999.0;

    void updateBounds() {
        minVal = parseWindowExpr(minWindowStr, 0.0);
        maxVal = parseWindowExpr(maxWindowStr, 999999.0);
    }
};

template <>
struct matjson::Serialize<LabelPreset> {
    static geode::Result<LabelPreset> fromJson(matjson::Value const& value) {
        if (!value.isObject()) return geode::Err("Expected object");
        LabelPreset p;
        p.id = value["id"].asInt().unwrapOr(1);
        p.swift = value["swift"].asInt().unwrapOr(0);
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
            {"swift", p.swift},
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
    int swift = 0;                                          // 动作自带的 Swift 维度，默认 0
};