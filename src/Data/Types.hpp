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
    int ifCount = 1;                                        // 所属 I/F 维度 (Inputs per Frame)
    double window = 1.0;
    cocos2d::ccColor4F color = { 1.f, 1.f, 1.f, 1.f };
    std::string customText = "";

    // 手动实现 operator==，解决 ccColor4F 导致 Clang 隐式删除比较操作符的问题
    bool operator==(const FrameWindowPreset& other) const {
        return ifCount == other.ifCount &&
            window == other.window &&
            color.r == other.color.r &&
            color.g == other.color.g &&
            color.b == other.color.b &&
            color.a == other.color.a &&
            customText == other.customText;
    }
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
        if (value.contains("if")) {
            p.ifCount = value["if"].asInt().unwrapOr(1);
        }
        else if (value.contains("swift")) {
            int oldSw = value["swift"].asInt().unwrapOr(0);
            p.ifCount = oldSw <= 0 ? 1 : (oldSw + 1);
        }
        else {
            p.ifCount = 1;
        }
        if (p.ifCount < 1) p.ifCount = 1;

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
            {"if", p.ifCount},
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
    bool useIF = false;                                     // 是否处于 I/F 模式（独立统计 I/F，而非 Frame Window）
    std::string minWindowStr = "";                          // Frame Window 最小值表达式
    std::string maxWindowStr = "";                          // Frame Window 最大值表达式
    std::string minIFStr = "";                              // I/F 最小值表达式
    std::string maxIFStr = "";                              // I/F 最大值表达式
    std::string text = "Label";
    std::string audioPath = "";
    cocos2d::ccColor4F color = { 1.f, 1.f, 1.f, 1.f };
    bool showInHud = true;
    double minVal = 0.0;
    double maxVal = 999999.0;

    void updateBounds() {
        if (useIF) {
            minVal = parseWindowExpr(minIFStr, 1.0);
            maxVal = parseWindowExpr(maxIFStr, 999999.0);
        }
        else {
            minVal = parseWindowExpr(minWindowStr, 0.0);
            maxVal = parseWindowExpr(maxWindowStr, 999999.0);
        }
    }

    // 手动实现 operator==，解决 ccColor4F 导致 Clang 隐式删除比较操作符的问题
    bool operator==(const LabelPreset& other) const {
        return id == other.id &&
            useIF == other.useIF &&
            minWindowStr == other.minWindowStr &&
            maxWindowStr == other.maxWindowStr &&
            minIFStr == other.minIFStr &&
            maxIFStr == other.maxIFStr &&
            text == other.text &&
            audioPath == other.audioPath &&
            color.r == other.color.r &&
            color.g == other.color.g &&
            color.b == other.color.b &&
            color.a == other.color.a &&
            showInHud == other.showInHud &&
            minVal == other.minVal &&
            maxVal == other.maxVal;
    }
};

template <>
struct matjson::Serialize<LabelPreset> {
    static geode::Result<LabelPreset> fromJson(matjson::Value const& value) {
        if (!value.isObject()) return geode::Err("Expected object");
        LabelPreset p;
        p.id = value["id"].asInt().unwrapOr(1);
        p.useIF = value["useIF"].asBool().unwrapOr(value["useSwift"].asBool().unwrapOr(false));
        p.minWindowStr = value["minW"].asString().unwrapOr("");
        p.maxWindowStr = value["maxW"].asString().unwrapOr("");
        p.minIFStr = value["minIF"].asString().unwrapOr(value["minSwift"].asString().unwrapOr(""));
        p.maxIFStr = value["maxIF"].asString().unwrapOr(value["maxSwift"].asString().unwrapOr(""));
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
            {"useIF", p.useIF},
            {"minW", p.minWindowStr},
            {"maxW", p.maxWindowStr},
            {"minIF", p.minIFStr},
            {"maxIF", p.maxIFStr},
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
    int ifCount = 1;                                        // 动作自带的 I/F，普通点击为 1
    bool isHold = false;                                    // true = Hold , false = Release
    bool isWindowEmpty = false;

    bool operator==(const FrameAction& other) const = default;
};