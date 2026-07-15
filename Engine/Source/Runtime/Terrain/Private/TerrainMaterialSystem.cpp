#include "Terrain/TerrainMaterialSystem.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::terrain {

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

} // namespace we::runtime::terrain
