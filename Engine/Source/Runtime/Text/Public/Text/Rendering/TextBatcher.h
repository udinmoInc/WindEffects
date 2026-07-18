#pragma once

#include "Text/Core/FontTypes.h"
#include "Text/Core/Types.h"
#include "Text/Export.h"
#include "Text/Layout/TextLayoutEngine.h"

#include <cstdint>
#include <span>
#include <vector>

namespace we::runtime::text::rendering {

struct TextVertex {
    float position[2]{};
    float uv[2]{};
    float color[4]{1.0f, 1.0f, 1.0f, 1.0f};
    float styleParams[4]{};
    uint32_t atlasPageIndex = 0;
};

struct PipelineStateKey {
    AtlasFormat format = AtlasFormat::Msdf;
    bool outline = false;
    bool shadow = false;

    bool operator==(const PipelineStateKey& other) const
    {
        return format == other.format && outline == other.outline && shadow == other.shadow;
    }
};

struct TextDrawBatch {
    uint64_t atlasKey = 0;
    AtlasPageHandle atlasPage = kInvalidAtlasPageHandle;
    PipelineStateKey pipelineKey{};
    std::span<const TextVertex> vertices;
    std::span<const uint32_t> indices;
    float msdfPixelRange = 4.0f;
};

struct TextBatcherStats {
    size_t drawCalls = 0;
    size_t vertexCount = 0;
    size_t indexCount = 0;
};

class TEXT_API ITextBatcher {
public:
    virtual ~ITextBatcher() = default;
    virtual void BeginFrame() = 0;
    virtual void AddLayout(const layout::LayoutResult& layout) = 0;
    [[nodiscard]] virtual std::vector<TextDrawBatch> BuildBatches() = 0;
    [[nodiscard]] virtual TextBatcherStats GetStats() const = 0;
};

[[nodiscard]] TEXT_API std::unique_ptr<ITextBatcher> CreateTextBatcher();

} // namespace we::runtime::text::rendering
