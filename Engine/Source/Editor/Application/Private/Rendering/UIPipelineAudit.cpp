#include "Rendering/UIPipelineAudit.h"
#include "Rendering/OverlayRenderer.h"
#include "Rendering/UIWidgetAdapter.h"
#include "Core/Logger.h"

#include <algorithm>
#include <limits>
#include <sstream>
#include <typeinfo>

namespace we::UI {

namespace {

constexpr uint64_t kDetailFrameLimit = 3;

const char* WidgetTypeName(const Widget& widget) {
#if defined(_MSC_VER)
    const char* raw = typeid(widget).name();
    if (raw != nullptr && raw[0] != '\0') {
        return raw;
    }
#endif
    return "Widget";
}

std::string RectToString(const Rect& rect) {
    return std::to_string(static_cast<int>(rect.x)) + "," +
           std::to_string(static_cast<int>(rect.y)) + " " +
           std::to_string(static_cast<int>(rect.width)) + "x" +
           std::to_string(static_cast<int>(rect.height));
}

void AuditVertexBounds(const std::vector<UIVertex2>& vertices, float& minX, float& minY, float& maxX, float& maxY) {
    minX = minY = std::numeric_limits<float>::max();
    maxX = maxY = std::numeric_limits<float>::lowest();
    for (const UIVertex2& v : vertices) {
        minX = std::min(minX, v.position[0]);
        minY = std::min(minY, v.position[1]);
        maxX = std::max(maxX, v.position[0]);
        maxY = std::max(maxY, v.position[1]);
    }
    if (vertices.empty()) {
        minX = minY = maxX = maxY = 0.0f;
    }
}

} // namespace

bool UIPipelineAudit::ShouldLogDetail(uint64_t frameNumber) {
    return frameNumber <= kDetailFrameLimit;
}

bool UIPipelineAudit::ShouldLogWidgetTree(uint64_t frameNumber) {
    return frameNumber <= kDetailFrameLimit;
}

void UIPipelineAudit::LogStage(uint64_t frameNumber, const char* stage, bool pass, const std::string& detail) {
    std::string line = std::string("[UIPipeline] Frame ") + std::to_string(frameNumber) + " | " + stage + " | " +
                       (pass ? "PASS" : "FAILED");
    if (!detail.empty()) {
        line += " | " + detail;
    }
    if (pass) {
        HE_INFO(line);
    } else {
        HE_ERROR(line);
    }
}

void UIPipelineAudit::LogWidgetNode(uint64_t frameNumber, const std::shared_ptr<Widget>& widget,
                                    const std::string& name, int depth, bool paintSubtreeInvoked) {
    if (!widget) {
        LogStage(frameNumber, "Widget Tree", false, name + " is null.");
        return;
    }

    const Rect geom = widget->GetGeometry();
    const auto& parent = widget->GetParent();
    std::string parentName = parent ? "attached" : "none";

    std::ostringstream oss;
    oss << name
        << " type=" << WidgetTypeName(*widget)
        << " visible=" << (widget->IsVisible() ? "yes" : "no")
        << " enabled=" << (widget->IsActive() ? "yes" : "no")
        << " geometry=" << RectToString(geom)
        << " children=" << widget->GetChildren().size()
        << " parent=" << parentName
        << " paintSubtreeInvoked=" << (paintSubtreeInvoked ? "yes" : "no");

    HE_INFO(std::string("[UIPipeline] Frame ") + std::to_string(frameNumber) + " | Widget | " + oss.str());
}

void UIPipelineAudit::WalkWidgetTree(uint64_t frameNumber, const std::shared_ptr<Widget>& root,
                                     const std::string& name, int depth, bool paintSubtreeInvoked) {
    if (!ShouldLogWidgetTree(frameNumber)) {
        return;
    }
    if (depth > 12) {
        return;
    }

    LogWidgetNode(frameNumber, root, name, depth, paintSubtreeInvoked && depth == 0);
    if (!root) {
        return;
    }

    const auto& children = root->GetChildren();
    for (size_t i = 0; i < children.size(); ++i) {
        WalkWidgetTree(frameNumber, children[i], name + ".child[" + std::to_string(i) + "]", depth + 1, paintSubtreeInvoked);
    }
}

void UIPipelineAudit::EmitCpuPipelineReport(uint64_t frameNumber, const UIPipelineCpuStats& stats) {
    const bool widgetTreePass = stats.rootWidgetExists && stats.widgetCount > 0;
    LogStage(frameNumber, "Widget Construction", stats.rootWidgetExists,
             stats.rootWidgetExists ? "root widget exists" : "root widget is null");

    LogStage(frameNumber, "Widget Tree", widgetTreePass,
             "widgets=" + std::to_string(stats.widgetCount) +
                 " visible=" + std::to_string(stats.visibleWidgetCount));

    const bool layoutPass = stats.targetWidth > 0 && stats.targetHeight > 0 && stats.layoutPassCount > 0;
    LogStage(frameNumber, "Layout Pass", layoutPass,
             layoutPass ? ("extent=" + std::to_string(stats.targetWidth) + "x" + std::to_string(stats.targetHeight))
                        : "layout pass did not run or target extent is zero");

    const bool paintPass = stats.paintCalls > 0;
    LogStage(frameNumber, "Paint Pass", paintPass,
             paintPass ? ("Paint() calls=" + std::to_string(stats.paintCalls) +
                          " paintCommands=" + std::to_string(stats.paintCommands))
                       : "Paint() was never called on root widget");

    const bool drawCommandsPass = stats.drawCommandsGenerated > 0;
    LogStage(frameNumber, "Generate Draw Commands", drawCommandsPass,
             drawCommandsPass ? ("drawCommands=" + std::to_string(stats.drawCommandsGenerated))
                              : "0 draw commands generated from Paint()");

    const bool geometryPass = stats.vertexCount > 4 && stats.indexCount > 0 && stats.batchCount > 0;
    LogStage(frameNumber, "Build Geometry", geometryPass,
             "vertices=" + std::to_string(stats.vertexCount) +
                 " indices=" + std::to_string(stats.indexCount) +
                 " batches=" + std::to_string(stats.batchCount));

    LogStage(frameNumber, "Vertex Buffer Upload", stats.geometryUploaded,
             stats.geometryUploaded ? "host-visible geometry buffers updated" : "UpdateGeometryBuffers() was not reached");

    if (ShouldLogDetail(frameNumber) && stats.vertexCount > 0) {
        float minX = 0.0f;
        float minY = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
        // Bounds are reported from the caller via GPU stats when available.
        (void)minX;
        (void)minY;
        (void)maxX;
        (void)maxY;
    }
}

void UIPipelineAudit::EmitGpuPipelineReport(uint64_t frameNumber, const UIPipelineGpuStats& stats) {
    LogStage(frameNumber, "Render Queue", stats.batchCount > 0,
             "batches=" + std::to_string(stats.batchCount));

    const bool drawLoopPass = stats.executedDrawCalls > 0;
    LogStage(frameNumber, "Draw Loop", drawLoopPass,
             "executedDrawCalls=" + std::to_string(stats.executedDrawCalls) +
                 " skippedDrawCalls=" + std::to_string(stats.skippedDrawCalls));

    LogStage(frameNumber, "vkCmdBindPipeline", stats.pipeline != VK_NULL_HANDLE,
             "pipeline=0x" + std::to_string(reinterpret_cast<uint64_t>(stats.pipeline)));

    LogStage(frameNumber, "vkCmdBindVertexBuffers", stats.vertexBuffer != VK_NULL_HANDLE,
             "vertexBuffer=0x" + std::to_string(reinterpret_cast<uint64_t>(stats.vertexBuffer)));

    LogStage(frameNumber, "vkCmdBindIndexBuffer", stats.indexBuffer != VK_NULL_HANDLE,
             "indexBuffer=0x" + std::to_string(reinterpret_cast<uint64_t>(stats.indexBuffer)));

    LogStage(frameNumber, "vkCmdDrawIndexed", drawLoopPass,
             "drawCalls=" + std::to_string(stats.executedDrawCalls));

    LogStage(frameNumber, "Current Command Buffer", stats.commandBuffer != VK_NULL_HANDLE,
             "cmd=0x" + std::to_string(reinterpret_cast<uint64_t>(stats.commandBuffer)));

    LogStage(frameNumber, "Current Swapchain Image", stats.swapchainImageIndex != UINT32_MAX,
             "image=" + std::to_string(stats.swapchainImageIndex));

    LogStage(frameNumber, "Viewport", stats.viewportWidth > 0 && stats.viewportHeight > 0,
             std::to_string(stats.viewportWidth) + "x" + std::to_string(stats.viewportHeight));

    if (ShouldLogDetail(frameNumber) && stats.vertices != nullptr && !stats.vertices->empty()) {
        float minX = 0.0f;
        float minY = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
        AuditVertexBounds(*stats.vertices, minX, minY, maxX, maxY);
        HE_INFO(std::string("[UIPipeline] Frame ") + std::to_string(frameNumber) +
                " | Vertex Bounds | min=(" + std::to_string(minX) + "," + std::to_string(minY) + ")" +
                " max=(" + std::to_string(maxX) + "," + std::to_string(maxY) + ")");
    }

    if (ShouldLogDetail(frameNumber) && stats.batches != nullptr && stats.vertices != nullptr) {
        const size_t sampleCount = std::min<size_t>(stats.batches->size(), 8);
        for (size_t i = 0; i < sampleCount; ++i) {
            const UIRenderBatch& batch = (*stats.batches)[i];
            uint32_t firstVertex = 0;
            if (!stats.vertices->empty() && batch.firstIndex < stats.vertices->size()) {
                firstVertex = batch.firstIndex;
            }
            const UIVertex2* sample = firstVertex < stats.vertices->size() ? &(*stats.vertices)[firstVertex] : nullptr;
            HE_INFO(std::string("[UIPipeline] Frame ") + std::to_string(frameNumber) +
                    " | Batch[" + std::to_string(i) + "]" +
                    " indexCount=" + std::to_string(batch.indexCount) +
                    " firstIndex=" + std::to_string(batch.firstIndex) +
                    " descriptor=0x" + std::to_string(reinterpret_cast<uint64_t>(batch.textureSet)) +
                    " scissor=" + std::to_string(static_cast<int>(batch.scissor[0])) + "," +
                    std::to_string(static_cast<int>(batch.scissor[1])) + " " +
                    std::to_string(static_cast<int>(batch.scissor[2])) + "x" +
                    std::to_string(static_cast<int>(batch.scissor[3])) +
                    (sample ? (" samplePos=(" + std::to_string(sample->position[0]) + "," +
                               std::to_string(sample->position[1]) + ")") : ""));
        }
    }
}

} // namespace we::UI
