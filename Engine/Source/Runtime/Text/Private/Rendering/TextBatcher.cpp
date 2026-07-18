#include "Text/Rendering/TextBatcher.h"

#include <algorithm>

namespace we::runtime::text::rendering {

namespace {

struct BatchBuilder {
    PipelineStateKey key{};
    AtlasPageHandle atlasPage = kInvalidAtlasPageHandle;
    std::vector<TextVertex> vertices;
    std::vector<uint32_t> indices;
    float msdfPixelRange = 4.0f;
};

} // namespace

class TextBatcher final : public ITextBatcher {
public:
    void BeginFrame() override
    {
        m_Builders.clear();
        m_Stats = {};
    }

    void AddLayout(const layout::LayoutResult& layout) override
    {
        for (const layout::PositionedGlyph& glyph : layout.glyphs) {
            if (!glyph.glyph.metrics.hasDrawableQuad) {
                continue;
            }

            PipelineStateKey key;
            key.format = AtlasFormat::Msdf;
            key.outline = glyph.hasOutline;
            key.shadow = glyph.hasShadow;

            BatchBuilder* builder = FindOrCreateBuilder(key, glyph.glyph.metrics.atlasPage, glyph.msdfPixelRange);
            const uint32_t base = static_cast<uint32_t>(builder->vertices.size());

            const float x0 = glyph.x;
            const float y0 = glyph.y;
            const float x1 = glyph.x + glyph.width;
            const float y1 = glyph.y + glyph.height;

            TextVertex vertices[4];
            vertices[0].position[0] = x0; vertices[0].position[1] = y0;
            vertices[0].uv[0] = glyph.glyph.metrics.atlasUv.u0; vertices[0].uv[1] = glyph.glyph.metrics.atlasUv.v0;
            vertices[1].position[0] = x1; vertices[1].position[1] = y0;
            vertices[1].uv[0] = glyph.glyph.metrics.atlasUv.u1; vertices[1].uv[1] = glyph.glyph.metrics.atlasUv.v0;
            vertices[2].position[0] = x1; vertices[2].position[1] = y1;
            vertices[2].uv[0] = glyph.glyph.metrics.atlasUv.u1; vertices[2].uv[1] = glyph.glyph.metrics.atlasUv.v1;
            vertices[3].position[0] = x0; vertices[3].position[1] = y1;
            vertices[3].uv[0] = glyph.glyph.metrics.atlasUv.u0; vertices[3].uv[1] = glyph.glyph.metrics.atlasUv.v1;

            for (auto& vertex : vertices) {
                vertex.color[0] = glyph.color.r;
                vertex.color[1] = glyph.color.g;
                vertex.color[2] = glyph.color.b;
                vertex.color[3] = glyph.color.a;
                vertex.atlasPageIndex = glyph.glyph.metrics.atlasPage;
                builder->vertices.push_back(vertex);
            }

            builder->indices.push_back(base + 0);
            builder->indices.push_back(base + 1);
            builder->indices.push_back(base + 2);
            builder->indices.push_back(base + 0);
            builder->indices.push_back(base + 2);
            builder->indices.push_back(base + 3);
        }
    }

    std::vector<TextDrawBatch> BuildBatches() override
    {
        std::vector<TextDrawBatch> batches;
        batches.reserve(m_Builders.size());

        for (auto& builder : m_Builders) {
            if (builder.vertices.empty()) {
                continue;
            }
            TextDrawBatch batch;
            batch.atlasPage = builder.atlasPage;
            batch.pipelineKey = builder.key;
            batch.msdfPixelRange = builder.msdfPixelRange;
            batch.vertices = std::span<const TextVertex>(builder.vertices.data(), builder.vertices.size());
            batch.indices = std::span<const uint32_t>(builder.indices.data(), builder.indices.size());
            batches.push_back(batch);
            m_Stats.drawCalls += 1;
            m_Stats.vertexCount += builder.vertices.size();
            m_Stats.indexCount += builder.indices.size();
        }

        return batches;
    }

    TextBatcherStats GetStats() const override
    {
        return m_Stats;
    }

private:
    BatchBuilder* FindOrCreateBuilder(
        const PipelineStateKey& key,
        const AtlasPageHandle atlasPage,
        const float msdfPixelRange)
    {
        for (auto& builder : m_Builders) {
            if (builder.key.format == key.format
                && builder.key.outline == key.outline
                && builder.key.shadow == key.shadow
                && builder.atlasPage == atlasPage) {
                return &builder;
            }
        }
        m_Builders.push_back(BatchBuilder{key, atlasPage, {}, {}, msdfPixelRange});
        return &m_Builders.back();
    }

    std::vector<BatchBuilder> m_Builders;
    TextBatcherStats m_Stats;
};

std::unique_ptr<ITextBatcher> CreateTextBatcher()
{
    return std::make_unique<TextBatcher>();
}

} // namespace we::runtime::text::rendering
