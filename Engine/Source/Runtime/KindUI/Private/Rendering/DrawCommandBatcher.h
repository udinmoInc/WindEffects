// ==============================================================================
// WindEffects — KindUI — DrawCommandBatcher
// Internal implementation for the KindUI module.
//
// Global (widget-agnostic) draw-command and batch coalescing for KindUI.
// Preserves paint order for alpha/text; compatible batches may merge across
// non-overlapping geometry only.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Core/PaintContext.h"
#include "KindUI/Host/OverlayRenderer.h"

#include <cstdint>
#include <vector>

namespace we::runtime::kindui {

struct DrawCommandBatcherStats {
    uint32_t commandsIn = 0;
    uint32_t commandsOut = 0;
    uint32_t commandsDropped = 0;
    uint32_t rectsMerged = 0;
    uint32_t clipsNormalized = 0;
    uint32_t iconsClustered = 0;
    uint32_t textsClustered = 0;
    uint32_t batchesIn = 0;
    uint32_t batchesOut = 0;
    uint32_t batchesMerged = 0;
    float commandCoalesceMs = 0.0f;
    float batchCoalesceMs = 0.0f;
};

/// Shared-path batcher: all widgets benefit without per-widget special cases.
class DrawCommandBatcher {
public:
    /// Cull empty/transparent cmds, normalize full-viewport clips, merge safe solid rects,
    /// and cluster non-overlapping same-clip icon runs by texture key for better batching.
    void CoalesceCommands(
        std::vector<DrawCommand>& commands,
        uint32_t viewportW,
        uint32_t viewportH,
        DrawCommandBatcherStats& stats);

    /// Order-preserving batch merge. Matching non-text batches may absorb into earlier
    /// state when intervening batch AABBs do not overlap (no pixel conflict).
    /// Text remains a hard wall.
    void CoalesceBatches(
        const std::vector<UIVertex2>& vertices,
        std::vector<uint32_t>& indices,
        std::vector<UIRenderBatch>& batches,
        DrawCommandBatcherStats& stats);

    [[nodiscard]] static bool IsEnabled();

private:
    struct IndexSpan {
        uint32_t begin = 0;
        uint32_t count = 0;
    };

    struct PendingBatch {
        UIRenderBatch header{};
        /// Spans into the pre-merge index buffer (and later into m_IndexScratch when rebuilt).
        /// Absorbing batches append spans instead of copying index bytes until the final flatten.
        std::vector<IndexSpan> spans;
        uint32_t indexCount = 0;
        float aabbMinX = 0.0f;
        float aabbMinY = 0.0f;
        float aabbMaxX = 0.0f;
        float aabbMaxY = 0.0f;
        bool aabbValid = false;
    };

    void ClusterIconRuns(std::vector<DrawCommand>& commands, DrawCommandBatcherStats& stats);
    void ClusterTextRuns(std::vector<DrawCommand>& commands, DrawCommandBatcherStats& stats);

    std::vector<DrawCommand> m_CmdScratch;
    std::vector<PendingBatch> m_Pending;
    std::vector<uint32_t> m_IndexScratch;
    std::vector<UIRenderBatch> m_BatchScratch;
};

} // namespace we::runtime::kindui
