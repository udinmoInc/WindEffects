#include "Rendering/Text/UiMsdfAudit.h"

#include "Core/Logger.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace we::UI::Text {

namespace {

bool SampleAtlas(const std::vector<uint8_t>& atlasRgba, int atlasWidth, int atlasHeight, float u, float v,
                 float& r, float& g, float& b) {
    if (atlasRgba.empty() || atlasWidth <= 0 || atlasHeight <= 0) {
        return false;
    }
    u = std::clamp(u, 0.0f, 1.0f);
    v = std::clamp(v, 0.0f, 1.0f);
    const float fx = u * static_cast<float>(atlasWidth - 1);
    const float fy = v * static_cast<float>(atlasHeight - 1);
    const int x0 = static_cast<int>(std::floor(fx));
    const int y0 = static_cast<int>(std::floor(fy));
    const int x1 = std::min(x0 + 1, atlasWidth - 1);
    const int y1 = std::min(y0 + 1, atlasHeight - 1);
    const float tx = fx - static_cast<float>(x0);
    const float ty = fy - static_cast<float>(y0);

    auto fetch = [&](int x, int y) {
        const size_t idx = static_cast<size_t>(y * atlasWidth + x) * 4;
        return std::array<float, 3>{
            atlasRgba[idx + 0] / 255.0f,
            atlasRgba[idx + 1] / 255.0f,
            atlasRgba[idx + 2] / 255.0f,
        };
    };

    const auto c00 = fetch(x0, y0);
    const auto c10 = fetch(x1, y0);
    const auto c01 = fetch(x0, y1);
    const auto c11 = fetch(x1, y1);
    r = (1.0f - tx) * (1.0f - ty) * c00[0] + tx * (1.0f - ty) * c10[0] + (1.0f - tx) * ty * c01[0]
        + tx * ty * c11[0];
    g = (1.0f - tx) * (1.0f - ty) * c00[1] + tx * (1.0f - ty) * c10[1] + (1.0f - tx) * ty * c01[1]
        + tx * ty * c11[1];
    b = (1.0f - tx) * (1.0f - ty) * c00[2] + tx * (1.0f - ty) * c10[2] + (1.0f - tx) * ty * c01[2]
        + tx * ty * c11[2];
    return true;
}

bool IsMsdfVertex(const UIVertex2& v) {
    const float type = v.sdfParams[1];
    return type > 2.5f && type < 3.5f;
}

bool IsInsideViewport(const UIVertex2& v, float viewportWidth, float viewportHeight) {
    return v.position[0] >= -1.0f && v.position[0] <= viewportWidth + 1.0f && v.position[1] >= -1.0f
        && v.position[1] <= viewportHeight + 1.0f;
}

} // namespace

float Median3(float r, float g, float b) {
    return std::max(std::min(r, g), std::min(std::max(r, g), b));
}

MsdfSampleResult EvaluateMsdfFragment(
    const UIVertex2& v0,
    const UIVertex2& v1,
    const UIVertex2& v2,
    const std::vector<uint8_t>& atlasRgba,
    int atlasWidth,
    int atlasHeight) {
    MsdfSampleResult result{};
    const float u = (v0.uv[0] + v1.uv[0] + v2.uv[0]) / 3.0f;
    const float v = (v0.uv[1] + v1.uv[1] + v2.uv[1]) / 3.0f;
    if (!SampleAtlas(atlasRgba, atlasWidth, atlasHeight, u, v, result.r, result.g, result.b)) {
        return result;
    }

    result.median = Median3(result.r, result.g, result.b);
    result.sigDist = result.median - 0.5f;

    const float pxRange = std::max(v0.sdfParams[2], 0.5f);
    const float screenW = std::max(v0.sdfRect[2], 1.0f);
    const float screenH = std::max(v0.sdfRect[3], 1.0f);
    const float unitRangeX = pxRange / screenW;
    const float unitRangeY = pxRange / screenH;
    const float du = std::max(std::fabs(v1.uv[0] - v0.uv[0]), 1e-6f);
    const float dv = std::max(std::fabs(v2.uv[1] - v0.uv[1]), 1e-6f);
    const float screenTexSizeX = 1.0f / du;
    const float screenTexSizeY = 1.0f / dv;
    const float screenPxRange = std::max(0.5f * (unitRangeX * screenTexSizeX + unitRangeY * screenTexSizeY), 1.0f);
    result.screenPxRange = screenPxRange;
    result.alpha = std::clamp(screenPxRange * result.sigDist + 0.5f, 0.0f, 1.0f);

    const float vertexAlpha = (v0.color[3] + v1.color[3] + v2.color[3]) / 3.0f;
    result.finalR = v0.color[0] * result.alpha;
    result.finalG = v0.color[1] * result.alpha;
    result.finalB = v0.color[2] * result.alpha;
    result.finalA = vertexAlpha * result.alpha;
    return result;
}

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
    uint32_t maxGlyphsToLog) {
    uint32_t textDrawCalls = 0;
    uint32_t textGlyphQuads = 0;
    uint32_t loggedGlyphs = 0;
    float minAlpha = 1.0f;
    float maxAlpha = 0.0f;
    float minMedian = 1.0f;
    float maxMedian = 0.0f;
    uint32_t zeroAlphaGlyphs = 0;
    uint32_t outOfViewportGlyphs = 0;

    for (const we::UI::UIRenderBatch& batch : batches) {
        if (batch.textureSet != fontDescriptorSet || batch.indexCount < 6) {
            continue;
        }
        ++textDrawCalls;

        for (uint32_t indexOffset = 0; indexOffset + 5 < batch.indexCount; indexOffset += 6) {
            const uint32_t baseIndex = batch.firstIndex + indexOffset;
            if (baseIndex + 5 >= indices.size()) {
                break;
            }
            const uint32_t i0 = indices[baseIndex + 0];
            const uint32_t i1 = indices[baseIndex + 1];
            const uint32_t i2 = indices[baseIndex + 2];
            if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) {
                continue;
            }
            const UIVertex2& v0 = vertices[i0];
            if (!IsMsdfVertex(v0)) {
                continue;
            }

            ++textGlyphQuads;
            const float quadW = std::max(v0.sdfRect[2], 0.0f);
            const float quadH = std::max(v0.sdfRect[3], 0.0f);
            if (!IsInsideViewport(v0, viewportWidth, viewportHeight)) {
                ++outOfViewportGlyphs;
            }

            const MsdfSampleResult sample = EvaluateMsdfFragment(v0, vertices[i1], vertices[i2], atlasRgba, atlasWidth, atlasHeight);
            minAlpha = std::min(minAlpha, sample.alpha);
            maxAlpha = std::max(maxAlpha, sample.alpha);
            minMedian = std::min(minMedian, sample.median);
            maxMedian = std::max(maxMedian, sample.median);
            if (sample.alpha <= 0.001f) {
                ++zeroAlphaGlyphs;
            }

            if (loggedGlyphs < maxGlyphsToLog) {
                HE_INFO(std::string("[UI MsdfAudit] glyph[") + std::to_string(loggedGlyphs)
                        + "] pos=(" + std::to_string(v0.position[0]) + "," + std::to_string(v0.position[1]) + ")"
                        + " size=" + std::to_string(quadW) + "x" + std::to_string(quadH)
                        + " uv=(" + std::to_string(v0.uv[0]) + "," + std::to_string(v0.uv[1]) + ")"
                        + " color=(" + std::to_string(v0.color[0]) + "," + std::to_string(v0.color[1]) + ","
                        + std::to_string(v0.color[2]) + "," + std::to_string(v0.color[3]) + ")"
                        + " msdf=(" + std::to_string(sample.r) + "," + std::to_string(sample.g) + ","
                        + std::to_string(sample.b) + ")"
                        + " median=" + std::to_string(sample.median)
                        + " sigDist=" + std::to_string(sample.sigDist)
                        + " pxRange=" + std::to_string(v0.sdfParams[2])
                        + " screenPxRange=" + std::to_string(sample.screenPxRange)
                        + " alpha=" + std::to_string(sample.alpha)
                        + " finalRGBA=(" + std::to_string(sample.finalR) + "," + std::to_string(sample.finalG) + ","
                        + std::to_string(sample.finalB) + "," + std::to_string(sample.finalA) + ")");
                ++loggedGlyphs;
            }
        }
    }

    HE_INFO(std::string("[UI MsdfAudit] textDrawCalls=") + std::to_string(textDrawCalls)
            + " glyphQuads=" + std::to_string(textGlyphQuads)
            + " median=[" + std::to_string(minMedian) + "," + std::to_string(maxMedian) + "]"
            + " alpha=[" + std::to_string(minAlpha) + "," + std::to_string(maxAlpha) + "]"
            + " zeroAlphaGlyphs=" + std::to_string(zeroAlphaGlyphs)
            + " outOfViewportGlyphs=" + std::to_string(outOfViewportGlyphs)
            + " atlas=" + std::to_string(atlasWidth) + "x" + std::to_string(atlasHeight)
            + " atlasBytes=" + std::to_string(atlasRgba.size()));
}

} // namespace we::UI::Text
