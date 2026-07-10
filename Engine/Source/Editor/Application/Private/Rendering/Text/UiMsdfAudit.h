#pragma once

#include "Rendering/OverlayRenderer.h"

#include <cstdint>
#include <vector>

namespace we::UI::Text {

struct MsdfSampleResult {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float median = 0.0f;
    float sigDist = 0.0f;
    float screenPxRange = 0.0f;
    float alpha = 0.0f;
    float finalR = 0.0f;
    float finalG = 0.0f;
    float finalB = 0.0f;
    float finalA = 0.0f;
};

float Median3(float r, float g, float b);

MsdfSampleResult EvaluateMsdfFragment(
    const UIVertex2& v0,
    const UIVertex2& v1,
    const UIVertex2& v2,
    const std::vector<uint8_t>& atlasRgba,
    int atlasWidth,
    int atlasHeight);

void AuditTextVertices(
    const std::vector<UIVertex2>& vertices,
    const std::vector<uint32_t>& indices,
    const std::vector<we::UI::UIRenderBatch>& batches,
    VkDescriptorSet fontDescriptorSet,
    const std::vector<uint8_t>& atlasRgba,
    int atlasWidth,
    int atlasHeight,
    float viewportWidth,
    float viewportHeight,
    uint32_t maxGlyphsToLog);

} // namespace we::UI::Text
