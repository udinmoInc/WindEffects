#pragma once

#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::editor::grid {

// Persistent editor grid settings loaded from Engine/Config/Editor/function.ini.
struct EditorGridConfig {
    bool enabled = true;
    bool snapOriginToCamera = true;
    bool enableFrustumCulling = true;
    bool enableDistanceCulling = true;
    bool antiAliasingEnabled = true;
    bool enableAxisLines = true;

    float renderRadius = 512.0f;
    float distanceCullRadius = 768.0f;
    float planeHeight = 0.0f;

    float level0Size = 1.0f;
    float level1Size = 10.0f;
    float level2Size = 100.0f;
    float level3Size = 1000.0f;

    float level0FadeStart = 25.0f;
    float level0FadeEnd = 80.0f;
    float level1FadeStart = 80.0f;
    float level1FadeEnd = 400.0f;
    float level2FadeStart = 400.0f;
    float level2FadeEnd = 4000.0f;
    float level3FadeStart = 100000.0f;
    float level3FadeEnd = 100000.0f;

    float lineThicknessMinor = 1.0f;
    float lineThicknessMajor = 1.1f;
    float lineThicknessAxis = 1.1f;
    float antiAliasScale = 1.0f;
    float baseOpacity = 1.0f;

    glm::vec4 minorGridColor{ 0.27f, 0.27f, 0.27f, 0.30f };
    glm::vec4 mediumGridColor{ 0.31f, 0.31f, 0.31f, 0.36f };
    glm::vec4 largeGridColor{ 0.34f, 0.34f, 0.34f, 0.40f };
    glm::vec4 majorGridColor{ 0.37f, 0.37f, 0.37f, 0.45f };
    glm::vec4 axisXColor{ 0.57f, 0.29f, 0.29f, 0.55f };
    glm::vec4 axisZColor{ 0.29f, 0.57f, 0.29f, 0.55f };

    bool axisBloom = false;
    float axisGlow = 0.0f;
    float axisEmissive = 0.0f;

    float depthBiasConstant = -2.0f;
    float depthBiasSlope = -1.0f;
    float depthOffset = 0.002f;
    float radiusFadeStart = 0.82f;
    float radiusFadeEnd = 1.0f;
};

class EditorFunctionConfig {
public:
    static EditorFunctionConfig& Get();

    void EnsureLoaded();
    void ReloadIfChanged();

    const EditorGridConfig& GetGridConfig() const { return m_GridConfig; }

private:
    EditorFunctionConfig() = default;

    std::filesystem::path GetConfigPath() const;
    std::unordered_map<std::string, std::string> LoadIniFile(const std::filesystem::path& path) const;
    void WriteIniFile(const std::filesystem::path& path, const std::unordered_map<std::string, std::string>& values) const;
    std::unordered_map<std::string, std::string> BuildDefaultEntries() const;
    void ApplyValues(const std::unordered_map<std::string, std::string>& values);
    void MergeMissingKeys(const std::filesystem::path& path, const std::unordered_map<std::string, std::string>& defaults);

    EditorGridConfig m_GridConfig{};
    std::filesystem::file_time_type m_LastWriteTime{};
    bool m_Loaded = false;
};

} // namespace we::editor::grid
