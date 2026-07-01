#include "EditorFunctionConfig.hpp"

#include "Core/EditorConfigPaths.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>

namespace we::editor::grid {

namespace {

std::string Trim(std::string value) {
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

bool ParseBool(const std::string& value, bool fallback) {
    const std::string lower = Trim(value);
    if (lower == "1" || lower == "true" || lower == "yes" || lower == "on") return true;
    if (lower == "0" || lower == "false" || lower == "no" || lower == "off") return false;
    return fallback;
}

int ParseInt(const std::string& value, int fallback) {
    try {
        return std::stoi(Trim(value));
    } catch (...) {
        return fallback;
    }
}

float ParseFloat(const std::string& value, float fallback) {
    try {
        return std::stof(Trim(value));
    } catch (...) {
        return fallback;
    }
}

glm::vec3 ParseColor(const std::string& value, const glm::vec3& fallback) {
    std::stringstream stream(Trim(value));
    std::string component;
    glm::vec3 color = fallback;
    int index = 0;
    while (std::getline(stream, component, ',') && index < 3) {
        color[index++] = ParseFloat(component, color[index]);
    }
    return glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f));
}

std::string BoolToString(bool value) { return value ? "1" : "0"; }

std::string FloatToString(float value) {
    std::ostringstream stream;
    stream.precision(8);
    stream << value;
    return stream.str();
}

glm::vec4 ParseColorRGBA(const std::string& value, const glm::vec4& fallback) {
    std::stringstream stream(Trim(value));
    std::string component;
    glm::vec4 color = fallback;
    int index = 0;
    while (std::getline(stream, component, ',') && index < 4) {
        color[index++] = ParseFloat(component, color[index]);
    }
    color.r = std::clamp(color.r, 0.0f, 1.0f);
    color.g = std::clamp(color.g, 0.0f, 1.0f);
    color.b = std::clamp(color.b, 0.0f, 1.0f);
    color.a = std::clamp(color.a, 0.0f, 1.0f);
    return color;
}

std::string ColorToString(const glm::vec3& color) {
    return FloatToString(color.r) + "," + FloatToString(color.g) + "," + FloatToString(color.b);
}

std::string ColorRGBAToString(const glm::vec4& color) {
    return FloatToString(color.r) + "," + FloatToString(color.g) + ","
        + FloatToString(color.b) + "," + FloatToString(color.a);
}

glm::vec4 ResolveGridColorRGBA(
    const std::unordered_map<std::string, std::string>& values,
    const char* rgbaKey,
    const char* legacyRgbKey,
    const char* legacyOpacityKey,
    const glm::vec4& fallback) {
    if (values.contains(rgbaKey)) {
        return ParseColorRGBA(values.at(rgbaKey), fallback);
    }
    if (legacyRgbKey != nullptr && legacyRgbKey[0] != '\0' && values.contains(legacyRgbKey)) {
        const glm::vec3 rgb = ParseColor(values.at(legacyRgbKey), glm::vec3(fallback));
        const float opacity = values.contains(legacyOpacityKey)
            ? std::clamp(ParseFloat(values.at(legacyOpacityKey), fallback.a), 0.0f, 1.0f)
            : fallback.a;
        return glm::vec4(rgb, opacity);
    }
    return fallback;
}

} // namespace

EditorFunctionConfig& EditorFunctionConfig::Get() {
    static EditorFunctionConfig instance;
    return instance;
}

std::filesystem::path EditorFunctionConfig::GetConfigPath() const {
    return we::core::ResolveEditorConfigPath("function.ini");
}

std::unordered_map<std::string, std::string> EditorFunctionConfig::BuildDefaultEntries() const {
    const EditorGridConfig defaults{};
    std::unordered_map<std::string, std::string> entries;
    entries.emplace("EditorGrid.Enabled", BoolToString(defaults.enabled));
    entries.emplace("EditorGrid.SnapOriginToCamera", BoolToString(defaults.snapOriginToCamera));
    entries.emplace("EditorGrid.EnableFrustumCulling", BoolToString(defaults.enableFrustumCulling));
    entries.emplace("EditorGrid.EnableDistanceCulling", BoolToString(defaults.enableDistanceCulling));
    entries.emplace("EditorGrid.AntiAliasingEnabled", BoolToString(defaults.antiAliasingEnabled));
    entries.emplace("EditorGrid.EnableAxisLines", BoolToString(defaults.enableAxisLines));
    entries.emplace("EditorGrid.RenderRadius", FloatToString(defaults.renderRadius));
    entries.emplace("EditorGrid.DistanceCullRadius", FloatToString(defaults.distanceCullRadius));
    entries.emplace("EditorGrid.PlaneHeight", FloatToString(defaults.planeHeight));
    entries.emplace("EditorGrid.Level0Size", FloatToString(defaults.level0Size));
    entries.emplace("EditorGrid.Level1Size", FloatToString(defaults.level1Size));
    entries.emplace("EditorGrid.Level2Size", FloatToString(defaults.level2Size));
    entries.emplace("EditorGrid.Level3Size", FloatToString(defaults.level3Size));
    entries.emplace("EditorGrid.Level0FadeStart", FloatToString(defaults.level0FadeStart));
    entries.emplace("EditorGrid.Level0FadeEnd", FloatToString(defaults.level0FadeEnd));
    entries.emplace("EditorGrid.Level1FadeStart", FloatToString(defaults.level1FadeStart));
    entries.emplace("EditorGrid.Level1FadeEnd", FloatToString(defaults.level1FadeEnd));
    entries.emplace("EditorGrid.Level2FadeStart", FloatToString(defaults.level2FadeStart));
    entries.emplace("EditorGrid.Level2FadeEnd", FloatToString(defaults.level2FadeEnd));
    entries.emplace("EditorGrid.Level3FadeStart", FloatToString(defaults.level3FadeStart));
    entries.emplace("EditorGrid.Level3FadeEnd", FloatToString(defaults.level3FadeEnd));
    entries.emplace("EditorGrid.MinorThickness", FloatToString(defaults.lineThicknessMinor));
    entries.emplace("EditorGrid.MajorThickness", FloatToString(defaults.lineThicknessMajor));
    entries.emplace("EditorGrid.AxisThickness", FloatToString(defaults.lineThicknessAxis));
    entries.emplace("EditorGrid.LineThicknessMinor", FloatToString(defaults.lineThicknessMinor));
    entries.emplace("EditorGrid.LineThicknessMajor", FloatToString(defaults.lineThicknessMajor));
    entries.emplace("EditorGrid.AntiAliasScale", FloatToString(defaults.antiAliasScale));
    entries.emplace("EditorGrid.BaseOpacity", FloatToString(defaults.baseOpacity));
    entries.emplace("EditorGrid.MinorGridColor", ColorRGBAToString(defaults.minorGridColor));
    entries.emplace("EditorGrid.MediumGridColor", ColorRGBAToString(defaults.mediumGridColor));
    entries.emplace("EditorGrid.LargeGridColor", ColorRGBAToString(defaults.largeGridColor));
    entries.emplace("EditorGrid.MajorGridColor", ColorRGBAToString(defaults.majorGridColor));
    entries.emplace("EditorGrid.AxisXColor", ColorRGBAToString(defaults.axisXColor));
    entries.emplace("EditorGrid.AxisZColor", ColorRGBAToString(defaults.axisZColor));
    entries.emplace("EditorGrid.AxisBloom", BoolToString(defaults.axisBloom));
    entries.emplace("EditorGrid.AxisGlow", FloatToString(defaults.axisGlow));
    entries.emplace("EditorGrid.AxisEmissive", FloatToString(defaults.axisEmissive));
    entries.emplace("EditorGrid.DepthBiasConstant", FloatToString(defaults.depthBiasConstant));
    entries.emplace("EditorGrid.DepthBiasSlope", FloatToString(defaults.depthBiasSlope));
    entries.emplace("EditorGrid.DepthOffset", FloatToString(defaults.depthOffset));
    entries.emplace("EditorGrid.RadiusFadeStart", FloatToString(defaults.radiusFadeStart));
    entries.emplace("EditorGrid.RadiusFadeEnd", FloatToString(defaults.radiusFadeEnd));
    return entries;
}

std::unordered_map<std::string, std::string> EditorFunctionConfig::LoadIniFile(const std::filesystem::path& path) const {
    std::unordered_map<std::string, std::string> values;
    std::ifstream file(path);
    if (!file.is_open()) {
        return values;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[') {
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = Trim(line.substr(0, separator));
        const std::string value = Trim(line.substr(separator + 1));
        if (!key.empty()) {
            values[key] = value;
        }
    }

    return values;
}

void EditorFunctionConfig::WriteIniFile(const std::filesystem::path& path,
                                        const std::unordered_map<std::string, std::string>& values) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        return;
    }

    file << "# WindEffects editor function configuration\n";
    file << "# Grid settings are hot-reloaded when this file changes.\n\n";
    file << "[EditorGrid]\n";

    std::vector<std::string> keys;
    keys.reserve(values.size());
    for (const auto& [key, _] : values) {
        keys.push_back(key);
    }
    std::sort(keys.begin(), keys.end());

    for (const std::string& key : keys) {
        file << key << "=" << values.at(key) << "\n";
    }
}

void EditorFunctionConfig::MergeMissingKeys(const std::filesystem::path& path,
                                            const std::unordered_map<std::string, std::string>& defaults) {
    auto values = LoadIniFile(path);
    bool changed = false;
    for (const auto& [key, value] : defaults) {
        if (!values.contains(key)) {
            values[key] = value;
            changed = true;
        }
    }
    if (changed || values.empty()) {
        WriteIniFile(path, values.empty() ? defaults : values);
    }
}

void EditorFunctionConfig::ApplyValues(const std::unordered_map<std::string, std::string>& values) {
    const auto defaults = BuildDefaultEntries();
    const auto get = [&](const char* key) -> std::string {
        if (values.contains(key)) return values.at(key);
        if (defaults.contains(key)) return defaults.at(key);
        return {};
    };

    EditorGridConfig config{};
    config.enabled = ParseBool(get("EditorGrid.Enabled"), config.enabled);
    config.snapOriginToCamera = ParseBool(get("EditorGrid.SnapOriginToCamera"), config.snapOriginToCamera);
    config.enableFrustumCulling = ParseBool(get("EditorGrid.EnableFrustumCulling"), config.enableFrustumCulling);
    config.enableDistanceCulling = ParseBool(get("EditorGrid.EnableDistanceCulling"), config.enableDistanceCulling);
    config.antiAliasingEnabled = ParseBool(get("EditorGrid.AntiAliasingEnabled"), config.antiAliasingEnabled);
    config.enableAxisLines = ParseBool(get("EditorGrid.EnableAxisLines"), config.enableAxisLines);

    config.renderRadius = std::max(16.0f, ParseFloat(get("EditorGrid.RenderRadius"), config.renderRadius));
    config.distanceCullRadius = std::max(config.renderRadius,
        ParseFloat(get("EditorGrid.DistanceCullRadius"), config.distanceCullRadius));
    config.planeHeight = ParseFloat(get("EditorGrid.PlaneHeight"), config.planeHeight);

    config.level0Size = std::max(0.001f, ParseFloat(get("EditorGrid.Level0Size"), config.level0Size));
    config.level1Size = std::max(config.level0Size, ParseFloat(get("EditorGrid.Level1Size"), config.level1Size));
    config.level2Size = std::max(config.level1Size, ParseFloat(get("EditorGrid.Level2Size"), config.level2Size));
    config.level3Size = std::max(config.level2Size, ParseFloat(get("EditorGrid.Level3Size"), config.level3Size));

    config.level0FadeStart = ParseFloat(get("EditorGrid.Level0FadeStart"), config.level0FadeStart);
    config.level0FadeEnd = std::max(config.level0FadeStart + 1.0f,
        ParseFloat(get("EditorGrid.Level0FadeEnd"), config.level0FadeEnd));
    config.level1FadeStart = ParseFloat(get("EditorGrid.Level1FadeStart"), config.level1FadeStart);
    config.level1FadeEnd = std::max(config.level1FadeStart + 1.0f,
        ParseFloat(get("EditorGrid.Level1FadeEnd"), config.level1FadeEnd));
    config.level2FadeStart = ParseFloat(get("EditorGrid.Level2FadeStart"), config.level2FadeStart);
    config.level2FadeEnd = std::max(config.level2FadeStart + 1.0f,
        ParseFloat(get("EditorGrid.Level2FadeEnd"), config.level2FadeEnd));
    config.level3FadeStart = ParseFloat(get("EditorGrid.Level3FadeStart"), config.level3FadeStart);
    config.level3FadeEnd = ParseFloat(get("EditorGrid.Level3FadeEnd"), config.level3FadeEnd);

    auto thickness = [&](const char* primary, const char* legacy, float fallback) -> float {
        if (values.contains(primary)) return ParseFloat(values.at(primary), fallback);
        if (legacy != nullptr && legacy[0] != '\0' && values.contains(legacy)) {
            return ParseFloat(values.at(legacy), fallback);
        }
        return fallback;
    };

    config.lineThicknessMinor = std::max(0.25f, thickness("EditorGrid.MinorThickness", "EditorGrid.LineThicknessMinor", config.lineThicknessMinor));
    config.lineThicknessMajor = std::max(config.lineThicknessMinor,
        thickness("EditorGrid.MajorThickness", "EditorGrid.LineThicknessMajor", config.lineThicknessMajor));
    config.lineThicknessAxis = std::max(config.lineThicknessMinor,
        thickness("EditorGrid.AxisThickness", "", config.lineThicknessAxis));
    config.lineThicknessAxis = std::min(config.lineThicknessAxis, config.lineThicknessMajor * 1.1f);
    config.lineThicknessMajor = std::min(config.lineThicknessMajor, config.lineThicknessMinor * 1.1f);

    config.antiAliasScale = std::max(0.1f, ParseFloat(get("EditorGrid.AntiAliasScale"), config.antiAliasScale));
    config.baseOpacity = std::clamp(ParseFloat(get("EditorGrid.BaseOpacity"), config.baseOpacity), 0.0f, 1.0f);

    config.minorGridColor = ResolveGridColorRGBA(
        values, "EditorGrid.MinorGridColor", "EditorGrid.MinorLineColor", "EditorGrid.MinorLineOpacity", config.minorGridColor);
    config.mediumGridColor = ResolveGridColorRGBA(
        values, "EditorGrid.MediumGridColor", "", "", config.mediumGridColor);
    config.largeGridColor = ResolveGridColorRGBA(
        values, "EditorGrid.LargeGridColor", "", "", config.largeGridColor);
    config.majorGridColor = ResolveGridColorRGBA(
        values, "EditorGrid.MajorGridColor", "EditorGrid.MajorLineColor", "EditorGrid.MajorLineOpacity", config.majorGridColor);

    auto loadAxisColor = [&](const char* key, const glm::vec4& fallback) -> glm::vec4 {
        if (!values.contains(key)) {
            return fallback;
        }
        const std::string& raw = values.at(key);
        if (std::count(raw.begin(), raw.end(), ',') >= 3) {
            return ParseColorRGBA(raw, fallback);
        }
        return glm::vec4(ParseColor(raw, glm::vec3(fallback)), fallback.a);
    };
    config.axisXColor = loadAxisColor("EditorGrid.AxisXColor", config.axisXColor);
    config.axisZColor = loadAxisColor("EditorGrid.AxisZColor", config.axisZColor);

    config.axisBloom = ParseBool(get("EditorGrid.AxisBloom"), config.axisBloom);
    config.axisGlow = std::max(0.0f, ParseFloat(get("EditorGrid.AxisGlow"), config.axisGlow));
    config.axisEmissive = std::max(0.0f, ParseFloat(get("EditorGrid.AxisEmissive"), config.axisEmissive));

    config.depthBiasConstant = ParseFloat(get("EditorGrid.DepthBiasConstant"), config.depthBiasConstant);
    config.depthBiasSlope = ParseFloat(get("EditorGrid.DepthBiasSlope"), config.depthBiasSlope);
    config.depthOffset = ParseFloat(get("EditorGrid.DepthOffset"), config.depthOffset);
    config.radiusFadeStart = std::clamp(ParseFloat(get("EditorGrid.RadiusFadeStart"), config.radiusFadeStart), 0.0f, 1.0f);
    config.radiusFadeEnd = std::clamp(ParseFloat(get("EditorGrid.RadiusFadeEnd"), config.radiusFadeEnd), config.radiusFadeStart, 1.0f);

    m_GridConfig = config;
}

void EditorFunctionConfig::EnsureLoaded() {
    const auto path = GetConfigPath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    const auto defaults = BuildDefaultEntries();
    if (!std::filesystem::exists(path)) {
        WriteIniFile(path, defaults);
    } else {
        MergeMissingKeys(path, defaults);
    }

    ApplyValues(LoadIniFile(path));
    if (std::filesystem::exists(path)) {
        m_LastWriteTime = std::filesystem::last_write_time(path, ec);
    }
    m_Loaded = true;
}

void EditorFunctionConfig::ReloadIfChanged() {
    if (!m_Loaded) {
        EnsureLoaded();
        return;
    }

    const auto path = GetConfigPath();
    if (!std::filesystem::exists(path)) {
        EnsureLoaded();
        return;
    }

    std::error_code ec;
    const auto writeTime = std::filesystem::last_write_time(path, ec);
    if (ec || writeTime == m_LastWriteTime) {
        return;
    }

    MergeMissingKeys(path, BuildDefaultEntries());
    ApplyValues(LoadIniFile(path));
    m_LastWriteTime = writeTime;
}

} // namespace we::editor::grid
