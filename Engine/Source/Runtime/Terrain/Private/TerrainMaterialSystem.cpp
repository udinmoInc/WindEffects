#include "Terrain/TerrainMaterialSystem.h"

#include "Core/Logger.h"
#include "Core/Paths.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace we::runtime::terrain {
namespace {

float ParseFloatAfterKey(const std::string& content, const char* key, float fallback) {
    const std::string needle = std::string("\"") + key + "\"";
    const auto pos = content.find(needle);
    if (pos == std::string::npos) {
        return fallback;
    }
    const auto colon = content.find(':', pos + needle.size());
    if (colon == std::string::npos) {
        return fallback;
    }
    try {
        return std::stof(content.substr(colon + 1));
    } catch (...) {
        return fallback;
    }
}

bool ParseVec3AfterKey(
    const std::string& content,
    const char* key,
    float& r,
    float& g,
    float& b)
{
    const std::string needle = std::string("\"") + key + "\"";
    const auto pos = content.find(needle);
    if (pos == std::string::npos) {
        return false;
    }
    const auto start = content.find('[', pos + needle.size());
    const auto end = content.find(']', start);
    if (start == std::string::npos || end == std::string::npos || end <= start) {
        return false;
    }
    std::stringstream parser(content.substr(start + 1, end - start - 1));
    char comma = 0;
    if (!(parser >> r)) {
        return false;
    }
    parser >> comma;
    if (!(parser >> g)) {
        return false;
    }
    parser >> comma;
    if (!(parser >> b)) {
        return false;
    }
    return true;
}

bool IsEditorLandscapeMaterialPath(const std::string& path) {
    return path.find("M_DefaultLandscapeEditor") != std::string::npos;
}

} // namespace

void TerrainMaterialSystem::Initialize(int width, int height) {
    m_Width = std::max(1, width);
    m_Height = std::max(1, height);
    m_Weights.assign(static_cast<std::size_t>(m_Width) * static_cast<std::size_t>(m_Height) * 4u, 0);
    // Default: full weight on layer 0 (R channel).
    for (std::size_t i = 0; i < m_Weights.size(); i += 4) {
        m_Weights[i] = 255;
    }
    m_Layers[0].name = "Base";
    m_Layers[1].name = "Rock";
    m_Layers[2].name = "Sand";
    m_Layers[3].name = "Snow";
    m_ActiveParams = TerrainMaterialParams{};
}

void TerrainMaterialSystem::Shutdown() {
    m_Weights.clear();
    m_Width = 0;
    m_Height = 0;
}

TerrainLayerDesc& TerrainMaterialSystem::Layer(int index) {
    const int i = std::clamp(index, 0, kMaxPaintLayers - 1);
    return m_Layers[static_cast<std::size_t>(i)];
}

const TerrainLayerDesc& TerrainMaterialSystem::Layer(int index) const {
    const int i = std::clamp(index, 0, kMaxPaintLayers - 1);
    return m_Layers[static_cast<std::size_t>(i)];
}

void TerrainMaterialSystem::PaintWeight(int x, int z, int layerIndex, float strength, float radius) {
    if (m_Weights.empty() || radius <= 0.0f) {
        return;
    }
    layerIndex = std::clamp(layerIndex, 0, kMaxPaintLayers - 1);
    strength = std::clamp(strength, 0.0f, 1.0f);
    const int r = static_cast<int>(std::ceil(radius));
    const int minX = std::max(0, x - r);
    const int maxX = std::min(m_Width - 1, x + r);
    const int minZ = std::max(0, z - r);
    const int maxZ = std::min(m_Height - 1, z + r);

    for (int pz = minZ; pz <= maxZ; ++pz) {
        for (int px = minX; px <= maxX; ++px) {
            const float dx = static_cast<float>(px - x);
            const float dz = static_cast<float>(pz - z);
            const float dist = std::sqrt(dx * dx + dz * dz);
            if (dist > radius) {
                continue;
            }
            const float w = (1.0f - dist / radius) * strength;
            std::uint8_t* pxWeights = &m_Weights[(static_cast<std::size_t>(pz)
                * static_cast<std::size_t>(m_Width) + static_cast<std::size_t>(px)) * 4u];

            float channels[4] = {
                pxWeights[0] / 255.0f,
                pxWeights[1] / 255.0f,
                pxWeights[2] / 255.0f,
                pxWeights[3] / 255.0f
            };
            channels[layerIndex] = std::clamp(channels[layerIndex] + w, 0.0f, 1.0f);

            float sum = channels[0] + channels[1] + channels[2] + channels[3];
            if (sum > 1e-6f) {
                for (float& c : channels) {
                    c /= sum;
                }
            } else {
                channels[0] = 1.0f;
            }

            for (int c = 0; c < 4; ++c) {
                pxWeights[c] = static_cast<std::uint8_t>(std::clamp(channels[c] * 255.0f, 0.0f, 255.0f));
            }
        }
    }

    NotifyVirtualTexturePagesDirty(minX, minZ, maxX, maxZ);
}

void TerrainMaterialSystem::EnsureDefaultLandscapeMaterial(TerrainCreateInfo& createInfo) {
    if (createInfo.materialSlot0.empty()) {
        createInfo.materialSlot0 = kDefaultLandscapeMaterialPath;
    }
    m_Layers[0].albedoPath = createInfo.materialSlot0;
    m_ActiveParams = ResolveShadingParams(createInfo);
    m_Layers[0].baseColor = m_ActiveParams.albedo;
    m_Layers[0].roughness = m_ActiveParams.roughness;
    m_Layers[0].metallic = m_ActiveParams.metallic;
}

bool TerrainMaterialSystem::TryLoadWemat(
    const std::filesystem::path& path,
    TerrainMaterialParams& outParams)
{
    std::ifstream in(path);
    if (!in) {
        return false;
    }
    std::stringstream ss;
    ss << in.rdbuf();
    const std::string content = ss.str();
    if (content.empty()) {
        return false;
    }

    float r = kDefaultLandscapeAlbedoR;
    float g = kDefaultLandscapeAlbedoG;
    float b = kDefaultLandscapeAlbedoB;
    if (ParseVec3AfterKey(content, "baseColor", r, g, b)) {
        outParams.albedo = we::math::Vec4(r, g, b, 1.0f);
    }
    outParams.roughness = std::clamp(
        ParseFloatAfterKey(content, "roughness", kDefaultLandscapeRoughness), 0.04f, 1.0f);
    outParams.metallic = std::clamp(
        ParseFloatAfterKey(content, "metallic", kDefaultLandscapeMetallic), 0.0f, 1.0f);
    const float opacity = std::clamp(ParseFloatAfterKey(content, "opacity", 1.0f), 0.0f, 1.0f);
    outParams.albedo.w = opacity;

    // Grid: editor default enables it; project materials stay grid-free unless specified.
    const bool editorDefault = IsEditorLandscapeMaterialPath(we::core::PathService::ToGeneric(path));
    const float defaultGridOpacity = editorDefault ? kDefaultLandscapeGridOpacity : 0.0f;
    outParams.gridSpacing = std::max(
        ParseFloatAfterKey(content, "gridSpacing", kDefaultLandscapeGridSpacing), 0.01f);
    outParams.gridLineWidth = std::max(
        ParseFloatAfterKey(content, "gridLineWidth", kDefaultLandscapeGridLineWidth), 0.01f);
    outParams.gridOpacity = std::clamp(
        ParseFloatAfterKey(content, "gridOpacity", defaultGridOpacity), 0.0f, 1.0f);

    float gr = kDefaultLandscapeGridColorR;
    float gg = kDefaultLandscapeGridColorG;
    float gb = kDefaultLandscapeGridColorB;
    if (ParseVec3AfterKey(content, "gridColor", gr, gg, gb)) {
        outParams.gridColor = we::math::Vec3(gr, gg, gb);
    } else {
        outParams.gridColor = we::math::Vec3(
            kDefaultLandscapeGridColorR,
            kDefaultLandscapeGridColorG,
            kDefaultLandscapeGridColorB);
    }
    outParams.gridFadeStart = kDefaultLandscapeGridFadeStart;
    outParams.gridFadeEnd = kDefaultLandscapeGridFadeEnd;

    outParams.materialPath = we::core::PathService::ToGeneric(path);
    outParams.loadedFromEngineContent = true;
    return true;
}

TerrainMaterialParams TerrainMaterialSystem::ResolveShadingParams(
    const TerrainCreateInfo& createInfo) const
{
    TerrainMaterialParams params{};
    params.materialPath = createInfo.materialSlot0.empty()
        ? kDefaultLandscapeMaterialPath
        : createInfo.materialSlot0;

    // Project materials always win when the assigned path exists.
    const auto tryPath = [&](const std::filesystem::path& candidate) -> bool {
        if (candidate.empty()) {
            return false;
        }
        return TryLoadWemat(candidate, params);
    };

    // Absolute / project-relative first.
    if (tryPath(params.materialPath)) {
        return params;
    }

    // Engine Content resolution for built-in default (and engine-relative slots).
    auto& paths = we::core::PathService::Get();
    const auto relative = (params.materialPath.find("Materials/") != std::string::npos)
        ? std::filesystem::path(params.materialPath.substr(params.materialPath.find("Materials/")))
        : std::filesystem::path(kDefaultLandscapeMaterialRelative);
    const auto candidates = paths.ResourceCandidates(relative);
    if (const auto found = we::core::PathService::FindExisting(candidates)) {
        if (tryPath(*found)) {
            return params;
        }
    }

    // Last resort: EngineRoot/Content and EngineRoot/Engine/Content.
    const auto engineRoot = paths.EngineRoot();
    if (tryPath(engineRoot / "Content" / kDefaultLandscapeMaterialRelative)
        || tryPath(paths.EngineContentRoot() / kDefaultLandscapeMaterialRelative))
    {
        return params;
    }

    // Embedded editor defaults — charcoal + world grid (never debug green).
    params.albedo = we::math::Vec4(
        kDefaultLandscapeAlbedoR,
        kDefaultLandscapeAlbedoG,
        kDefaultLandscapeAlbedoB,
        1.0f);
    params.roughness = kDefaultLandscapeRoughness;
    params.metallic = kDefaultLandscapeMetallic;
    params.gridSpacing = kDefaultLandscapeGridSpacing;
    params.gridLineWidth = kDefaultLandscapeGridLineWidth;
    params.gridColor = we::math::Vec3(
        kDefaultLandscapeGridColorR,
        kDefaultLandscapeGridColorG,
        kDefaultLandscapeGridColorB);
    params.gridOpacity = kDefaultLandscapeGridOpacity;
    params.gridFadeStart = kDefaultLandscapeGridFadeStart;
    params.gridFadeEnd = kDefaultLandscapeGridFadeEnd;
    params.materialPath = kDefaultLandscapeMaterialPath;
    params.loadedFromEngineContent = false;
    HE_WARN(
        "[Terrain] M_DefaultLandscapeEditor.wemat not found; using embedded editor defaults.");
    return params;
}

} // namespace we::runtime::terrain
