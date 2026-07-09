#pragma once

#include "Application/Export.h"
#include "Core/Widget.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <volk.h>

namespace we::UI {

struct UIRenderBatch;
struct UIVertex2;

struct UIPipelineCpuStats {
    bool rootWidgetExists = false;
    uint32_t widgetCount = 0;
    uint32_t visibleWidgetCount = 0;
    uint32_t layoutPassCount = 0;
    uint32_t paintCalls = 0;
    uint32_t paintCommands = 0;
    uint32_t drawCommandsGenerated = 0;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    uint32_t batchCount = 0;
    uint32_t targetWidth = 0;
    uint32_t targetHeight = 0;
    bool geometryUploaded = false;
};

struct UIPipelineGpuStats {
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    uint32_t swapchainImageIndex = UINT32_MAX;
    uint32_t executedDrawCalls = 0;
    uint32_t skippedDrawCalls = 0;
    uint32_t batchCount = 0;
    uint32_t viewportWidth = 0;
    uint32_t viewportHeight = 0;
    const std::vector<UIVertex2>* vertices = nullptr;
    const std::vector<UIRenderBatch>* batches = nullptr;
};

class APPLICATION_API UIPipelineAudit {
public:
    static bool ShouldLogDetail(uint64_t frameNumber);
    static bool ShouldLogWidgetTree(uint64_t frameNumber);

    static void LogStage(uint64_t frameNumber, const char* stage, bool pass, const std::string& detail = {});
    static void LogWidgetNode(uint64_t frameNumber, const std::shared_ptr<Widget>& widget,
                              const std::string& name, int depth, bool paintSubtreeInvoked);
    static void WalkWidgetTree(uint64_t frameNumber, const std::shared_ptr<Widget>& root,
                               const std::string& name, int depth, bool paintSubtreeInvoked);

    static void EmitCpuPipelineReport(uint64_t frameNumber, const UIPipelineCpuStats& stats);
    static void EmitGpuPipelineReport(uint64_t frameNumber, const UIPipelineGpuStats& stats);
};

} // namespace we::UI
