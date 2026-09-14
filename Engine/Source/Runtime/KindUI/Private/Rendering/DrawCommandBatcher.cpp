// ==============================================================================
// WindEffects — KindUI — DrawCommandBatcher
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Rendering/DrawCommandBatcher.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>

namespace we::runtime::kindui {
namespace {

constexpr float kColorEps = 0.001f;
constexpr float kRadiusEps = 0.01f;
constexpr float kGapEps = 0.6f; // allow 0.5px AA gaps when merging adjacent rects

bool EnvDisabled() {
    const char* v = std::getenv("WE_UI_DISABLE_GLOBAL_BATCH");
    return v != nullptr && v[0] != '\0' && v[0] != '0';
}

bool ColorsEqual(const Color& a, const Color& b) {
    return std::fabs(a.r - b.r) < kColorEps
        && std::fabs(a.g - b.g) < kColorEps
        && std::fabs(a.b - b.b) < kColorEps
        && std::fabs(a.a - b.a) < kColorEps;
}

bool ClipsEqual(const Rect& a, const Rect& b) {
    return std::fabs(a.x - b.x) < 0.5f
        && std::fabs(a.y - b.y) < 0.5f
        && std::fabs(a.width - b.width) < 0.5f
        && std::fabs(a.height - b.height) < 0.5f;
}

bool RectContainsRect(const Rect& outer, const Rect& inner) {
    return inner.x >= outer.x - kGapEps
        && inner.y >= outer.y - kGapEps
        && (inner.x + inner.width) <= (outer.x + outer.width) + kGapEps
        && (inner.y + inner.height) <= (outer.y + outer.height) + kGapEps;
}

bool RectsOverlap(const Rect& a, const Rect& b) {
    return a.x < b.x + b.width && b.x < a.x + a.width
        && a.y < b.y + b.height && b.y < a.y + a.height;
}

bool TryMergeSolidRects(Rect& a, const Rect& b) {
    if (RectContainsRect(a, b)) {
        return true;
    }
    if (RectContainsRect(b, a)) {
        a = b;
        return true;
    }

    const float ax1 = a.x;
    const float ay1 = a.y;
    const float ax2 = a.x + a.width;
    const float ay2 = a.y + a.height;
    const float bx1 = b.x;
    const float by1 = b.y;
    const float bx2 = b.x + b.width;
    const float by2 = b.y + b.height;

    if (std::fabs(ay1 - by1) <= kGapEps && std::fabs(ay2 - by2) <= kGapEps) {
        if (bx1 <= ax2 + kGapEps && ax1 <= bx2 + kGapEps) {
            const float nx1 = (std::min)(ax1, bx1);
            const float nx2 = (std::max)(ax2, bx2);
            a.x = nx1;
            a.width = nx2 - nx1;
            return true;
        }
    }

    if (std::fabs(ax1 - bx1) <= kGapEps && std::fabs(ax2 - bx2) <= kGapEps) {
        if (by1 <= ay2 + kGapEps && ay1 <= by2 + kGapEps) {
            const float ny1 = (std::min)(ay1, by1);
            const float ny2 = (std::max)(ay2, by2);
            a.y = ny1;
            a.height = ny2 - ny1;
            return true;
        }
    }

    return false;
}

bool IsMergeableSolidRect(const DrawCommand& cmd) {
    return cmd.type == DrawCommandType::Rect
        && cmd.borderRadius <= kRadiusEps
        && cmd.color.a > kColorEps
        && cmd.rect.width > 0.0f
        && cmd.rect.height > 0.0f;
}

bool CanMergeRectCommands(const DrawCommand& a, const DrawCommand& b) {
    if (!IsMergeableSolidRect(a) || !IsMergeableSolidRect(b)) {
        return false;
    }
    if (!ColorsEqual(a.color, b.color)) {
        return false;
    }
    if (!ClipsEqual(a.clipRect, b.clipRect)) {
        return false;
    }
    return true;
}

bool NormalizeClip(Rect& clip, uint32_t viewportW, uint32_t viewportH) {
    if (viewportW == 0 || viewportH == 0) {
        return false;
    }
    const float vw = static_cast<float>(viewportW);
    const float vh = static_cast<float>(viewportH);

    if (clip.width >= vw * 0.999f && clip.height >= vh * 0.999f
        && clip.x <= 0.5f && clip.y <= 0.5f) {
        if (clip.x != 0.0f || clip.y != 0.0f || clip.width != vw || clip.height != vh) {
            clip = {0.0f, 0.0f, vw, vh};
            return true;
        }
        return false;
    }

    const Rect viewport{0.0f, 0.0f, vw, vh};
    const Rect clamped = clip.Intersect(viewport);
    if (clamped != clip) {
        clip = clamped;
        return true;
    }
    return false;
}

bool CommandVisible(const DrawCommand& cmd) {
    if (cmd.clipRect.width <= 0.0f || cmd.clipRect.height <= 0.0f) {
        return false;
    }
    if (cmd.color.a <= 0.0f && cmd.type != DrawCommandType::Texture
        && cmd.type != DrawCommandType::ColorTexture
        && cmd.type != DrawCommandType::Icon) {
        return false;
    }
    switch (cmd.type) {
    case DrawCommandType::Rect:
    case DrawCommandType::Gradient:
    case DrawCommandType::Shadow:
    case DrawCommandType::RoundedOutline:
    case DrawCommandType::Texture:
    case DrawCommandType::ColorTexture:
    case DrawCommandType::Icon:
        return cmd.rect.width > 0.0f && cmd.rect.height > 0.0f;
    case DrawCommandType::Line:
        return true;
    case DrawCommandType::Text:
        return !cmd.text.empty();
    }
    return true;
}

bool SameBatchKey(const UIRenderBatch& a, const UIRenderBatch& b) {
    if (a.textureSet != b.textureSet
        || a.isText != b.isText
        || a.opaqueReplace != b.opaqueReplace
        || a.stencilRef != b.stencilRef) {
        return false;
    }
    if (std::memcmp(a.scissor, b.scissor, sizeof(a.scissor)) != 0) {
        return false;
    }
    if (a.isText) {
        return a.atlasWidth == b.atlasWidth
            && a.atlasHeight == b.atlasHeight
            && a.msdfPixelRange == b.msdfPixelRange;
    }
    return true;
}

bool AabbOverlap(
    float aminX, float aminY, float amaxX, float amaxY,
    float bminX, float bminY, float bmaxX, float bmaxY)
{
    return aminX < bmaxX && bminX < amaxX && aminY < bmaxY && bminY < amaxY;
}

bool ComputeBatchAabb(
    const std::vector<UIVertex2>& vertices,
    const uint32_t* indices,
    uint32_t indexCount,
    float& minX,
    float& minY,
    float& maxX,
    float& maxY)
{
    if (!indices || indexCount == 0 || vertices.empty()) {
        return false;
    }
    minX = std::numeric_limits<float>::max();
    minY = std::numeric_limits<float>::max();
    maxX = -std::numeric_limits<float>::max();
    maxY = -std::numeric_limits<float>::max();
    bool any = false;
    for (uint32_t i = 0; i < indexCount; ++i) {
        const uint32_t vi = indices[i];
        if (vi >= vertices.size()) {
            continue;
        }
        const float x = vertices[vi].position[0];
        const float y = vertices[vi].position[1];
        minX = (std::min)(minX, x);
        minY = (std::min)(minY, y);
        maxX = (std::max)(maxX, x);
        maxY = (std::max)(maxY, y);
        any = true;
    }
    return any;
}

} // namespace

bool DrawCommandBatcher::IsEnabled() {
    static const bool enabled = !EnvDisabled();
    return enabled;
}

void DrawCommandBatcher::ClusterIconRuns(
    std::vector<DrawCommand>& commands,
    DrawCommandBatcherStats& stats)
{
    size_t i = 0;
    while (i < commands.size()) {
        if (commands[i].type != DrawCommandType::Icon) {
            ++i;
            continue;
        }

        size_t j = i + 1;
        while (j < commands.size()
            && commands[j].type == DrawCommandType::Icon
            && ClipsEqual(commands[i].clipRect, commands[j].clipRect)) {
            ++j;
        }

        const size_t runLen = j - i;
        if (runLen >= 2) {
            bool anyOverlap = false;
            for (size_t a = i; a < j && !anyOverlap; ++a) {
                for (size_t b = a + 1; b < j; ++b) {
                    if (RectsOverlap(commands[a].rect, commands[b].rect)) {
                        anyOverlap = true;
                        break;
                    }
                }
            }

            if (!anyOverlap) {
                std::stable_sort(
                    commands.begin() + static_cast<std::ptrdiff_t>(i),
                    commands.begin() + static_cast<std::ptrdiff_t>(j),
                    [](const DrawCommand& a, const DrawCommand& b) {
                        if (a.iconSizePx != b.iconSizePx) {
                            return a.iconSizePx < b.iconSizePx;
                        }
                        return a.iconStem < b.iconStem;
                    });
                stats.iconsClustered += static_cast<uint32_t>(runLen);
            }
        }
        i = j;
    }
}

void DrawCommandBatcher::ClusterTextRuns(
    std::vector<DrawCommand>& commands,
    DrawCommandBatcherStats& stats)
{
    size_t i = 0;
    while (i < commands.size()) {
        if (commands[i].type != DrawCommandType::Text) {
            ++i;
            continue;
        }

        size_t j = i + 1;
        while (j < commands.size()
            && commands[j].type == DrawCommandType::Text
            && ClipsEqual(commands[i].clipRect, commands[j].clipRect)) {
            ++j;
        }

        const size_t runLen = j - i;
        if (runLen >= 2) {
            bool anyOverlap = false;
            for (size_t a = i; a < j && !anyOverlap; ++a) {
                const Rect aBounds{
                    commands[a].rect.x,
                    commands[a].rect.y,
                    std::max(commands[a].rect.width, commands[a].fontSize * 8.0f),
                    std::max(commands[a].rect.height, commands[a].fontSize * 1.4f)
                };
                for (size_t b = a + 1; b < j; ++b) {
                    const Rect bBounds{
                        commands[b].rect.x,
                        commands[b].rect.y,
                        std::max(commands[b].rect.width, commands[b].fontSize * 8.0f),
                        std::max(commands[b].rect.height, commands[b].fontSize * 1.4f)
                    };
                    if (RectsOverlap(aBounds, bBounds)) {
                        anyOverlap = true;
                        break;
                    }
                }
            }

            if (!anyOverlap) {
                std::stable_sort(
                    commands.begin() + static_cast<std::ptrdiff_t>(i),
                    commands.begin() + static_cast<std::ptrdiff_t>(j),
                    [](const DrawCommand& a, const DrawCommand& b) {
                        if (a.fontSize != b.fontSize) {
                            return a.fontSize < b.fontSize;
                        }
                        if (a.textWeight != b.textWeight) {
                            return a.textWeight < b.textWeight;
                        }
                        return a.textItalic < b.textItalic;
                    });
                stats.textsClustered += static_cast<uint32_t>(runLen);
            }
        }
        i = j;
    }
}

void DrawCommandBatcher::CoalesceCommands(
    std::vector<DrawCommand>& commands,
    uint32_t viewportW,
    uint32_t viewportH,
    DrawCommandBatcherStats& stats)
{
    using clock = std::chrono::steady_clock;
    const auto t0 = clock::now();

    stats.commandsIn = static_cast<uint32_t>(commands.size());
    stats.commandsDropped = 0;
    stats.rectsMerged = 0;
    stats.clipsNormalized = 0;
    stats.iconsClustered = 0;
    stats.textsClustered = 0;

    if (commands.empty()) {
        stats.commandsOut = 0;
        stats.commandCoalesceMs = 0.0f;
        return;
    }

    m_CmdScratch.clear();
    if (m_CmdScratch.capacity() < commands.size()) {
        m_CmdScratch.reserve(commands.size());
    }

    for (DrawCommand& cmd : commands) {
        if (NormalizeClip(cmd.clipRect, viewportW, viewportH)) {
            ++stats.clipsNormalized;
        }
        if (!CommandVisible(cmd)) {
            ++stats.commandsDropped;
            continue;
        }

        if (cmd.type == DrawCommandType::Rect
            || cmd.type == DrawCommandType::Gradient
            || cmd.type == DrawCommandType::Shadow
            || cmd.type == DrawCommandType::RoundedOutline
            || cmd.type == DrawCommandType::Texture
            || cmd.type == DrawCommandType::ColorTexture
            || cmd.type == DrawCommandType::Icon) {
            const Rect clipped = cmd.rect.Intersect(cmd.clipRect);
            if (clipped.IsEmpty()) {
                ++stats.commandsDropped;
                continue;
            }
            if (cmd.type == DrawCommandType::Rect && cmd.borderRadius <= kRadiusEps) {
                cmd.rect = clipped;
            }
        }

        if (!m_CmdScratch.empty() && CanMergeRectCommands(m_CmdScratch.back(), cmd)) {
            if (TryMergeSolidRects(m_CmdScratch.back().rect, cmd.rect)) {
                ++stats.rectsMerged;
                continue;
            }
        }

        m_CmdScratch.push_back(std::move(cmd));
    }

    ClusterIconRuns(m_CmdScratch, stats);
    ClusterTextRuns(m_CmdScratch, stats);

    commands.swap(m_CmdScratch);
    m_CmdScratch.clear();

    stats.commandsOut = static_cast<uint32_t>(commands.size());
    stats.commandCoalesceMs = static_cast<float>(
        std::chrono::duration<double, std::milli>(clock::now() - t0).count());
}

void DrawCommandBatcher::CoalesceBatches(
    const std::vector<UIVertex2>& vertices,
    std::vector<uint32_t>& indices,
    std::vector<UIRenderBatch>& batches,
    DrawCommandBatcherStats& stats)
{
    using clock = std::chrono::steady_clock;
    const auto t0 = clock::now();

    stats.batchesIn = static_cast<uint32_t>(batches.size());
    stats.batchesMerged = 0;

    if (batches.size() <= 1) {
        stats.batchesOut = stats.batchesIn;
        stats.batchCoalesceMs = 0.0f;
        return;
    }

    // Spans refer into `indices` until the final flatten; do not mutate indices mid-pass.
    m_Pending.clear();
    if (m_Pending.capacity() < batches.size()) {
        m_Pending.reserve(batches.size());
    }

    auto appendSpan = [](PendingBatch& dest, uint32_t begin, uint32_t count) {
        dest.spans.push_back(IndexSpan{begin, count});
        dest.indexCount += count;
    };

    auto expandAabb = [](PendingBatch& dest, bool aabbOk, float minX, float minY, float maxX, float maxY) {
        if (!aabbOk) {
            return;
        }
        if (!dest.aabbValid) {
            dest.aabbMinX = minX;
            dest.aabbMinY = minY;
            dest.aabbMaxX = maxX;
            dest.aabbMaxY = maxY;
            dest.aabbValid = true;
        } else {
            dest.aabbMinX = (std::min)(dest.aabbMinX, minX);
            dest.aabbMinY = (std::min)(dest.aabbMinY, minY);
            dest.aabbMaxX = (std::max)(dest.aabbMaxX, maxX);
            dest.aabbMaxY = (std::max)(dest.aabbMaxY, maxY);
        }
    };

    for (const UIRenderBatch& batch : batches) {
        if (batch.indexCount == 0) {
            continue;
        }
        if (batch.firstIndex + batch.indexCount > indices.size()) {
            continue;
        }

        const uint32_t* src = indices.data() + batch.firstIndex;
        float minX = 0.0f, minY = 0.0f, maxX = 0.0f, maxY = 0.0f;
        const bool aabbOk = ComputeBatchAabb(vertices, src, batch.indexCount, minX, minY, maxX, maxY);

        bool absorbed = false;

        if (!m_Pending.empty() && SameBatchKey(m_Pending.back().header, batch)) {
            auto& dest = m_Pending.back();
            appendSpan(dest, batch.firstIndex, batch.indexCount);
            expandAabb(dest, aabbOk, minX, minY, maxX, maxY);
            ++stats.batchesMerged;
            absorbed = true;
        } else if (batch.isText) {
            // Text may absorb into an earlier matching text batch when intervening
            // opaque non-text batches do not overlap (preserves alpha/text order elsewhere).
            for (int j = static_cast<int>(m_Pending.size()) - 1; j >= 0; --j) {
                PendingBatch& cand = m_Pending[static_cast<size_t>(j)];
                if (cand.header.isText && SameBatchKey(cand.header, batch)) {
                    if (!aabbOk || !cand.aabbValid
                        || !AabbOverlap(
                            cand.aabbMinX, cand.aabbMinY, cand.aabbMaxX, cand.aabbMaxY,
                            minX, minY, maxX, maxY)) {
                        appendSpan(cand, batch.firstIndex, batch.indexCount);
                        expandAabb(cand, aabbOk, minX, minY, maxX, maxY);
                        ++stats.batchesMerged;
                        absorbed = true;
                    }
                    break;
                }
                if (cand.header.isText || !cand.header.opaqueReplace) {
                    break;
                }
                if (!aabbOk || !cand.aabbValid
                    || AabbOverlap(
                        cand.aabbMinX, cand.aabbMinY, cand.aabbMaxX, cand.aabbMaxY,
                        minX, minY, maxX, maxY)) {
                    break;
                }
            }
        } else if (batch.opaqueReplace && !batch.isText) {
            // Opaque-only scan-back. Alpha/icon reorder across intervening draws is unsafe
            // when scissors are large even if AABBs appear disjoint (clip + blend).
            for (int j = static_cast<int>(m_Pending.size()) - 1; j >= 0; --j) {
                PendingBatch& cand = m_Pending[static_cast<size_t>(j)];
                if (cand.header.isText || !cand.header.opaqueReplace) {
                    break; // text/alpha wall
                }
                if (SameBatchKey(cand.header, batch)) {
                    appendSpan(cand, batch.firstIndex, batch.indexCount);
                    expandAabb(cand, aabbOk, minX, minY, maxX, maxY);
                    ++stats.batchesMerged;
                    absorbed = true;
                    break;
                }
                if (!aabbOk || !cand.aabbValid
                    || AabbOverlap(
                        cand.aabbMinX, cand.aabbMinY, cand.aabbMaxX, cand.aabbMaxY,
                        minX, minY, maxX, maxY)) {
                    break;
                }
            }
        }

        if (!absorbed) {
            PendingBatch pending;
            pending.header = batch;
            pending.spans.clear();
            pending.indexCount = 0;
            if (pending.spans.capacity() < 4) {
                pending.spans.reserve(4);
            }
            appendSpan(pending, batch.firstIndex, batch.indexCount);
            pending.aabbValid = aabbOk;
            pending.aabbMinX = minX;
            pending.aabbMinY = minY;
            pending.aabbMaxX = maxX;
            pending.aabbMaxY = maxY;
            m_Pending.push_back(std::move(pending));
        }
    }

    m_IndexScratch.clear();
    m_BatchScratch.clear();
    size_t totalIndices = 0;
    for (const PendingBatch& p : m_Pending) {
        totalIndices += p.indexCount;
    }
    if (m_IndexScratch.capacity() < totalIndices) {
        m_IndexScratch.reserve(totalIndices);
    }
    if (m_BatchScratch.capacity() < m_Pending.size()) {
        m_BatchScratch.reserve(m_Pending.size());
    }

    for (PendingBatch& p : m_Pending) {
        UIRenderBatch out = p.header;
        out.firstIndex = static_cast<uint32_t>(m_IndexScratch.size());
        out.indexCount = p.indexCount;
        for (const IndexSpan& span : p.spans) {
            const uint32_t* begin = indices.data() + span.begin;
            m_IndexScratch.insert(m_IndexScratch.end(), begin, begin + span.count);
        }
        m_BatchScratch.push_back(out);
        p.spans.clear();
        p.indexCount = 0;
    }

    indices.swap(m_IndexScratch);
    batches.swap(m_BatchScratch);
    m_IndexScratch.clear();
    m_BatchScratch.clear();
    m_Pending.clear();

    stats.batchesOut = static_cast<uint32_t>(batches.size());
    stats.batchCoalesceMs = static_cast<float>(
        std::chrono::duration<double, std::milli>(clock::now() - t0).count());
}

} // namespace we::runtime::kindui
