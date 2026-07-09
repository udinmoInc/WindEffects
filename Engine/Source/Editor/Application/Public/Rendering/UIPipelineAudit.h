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

// Evidence-based validation results for each pipeline stage
struct StageValidation {
    bool executed = false;
    bool succeeded = false;
    std::string failureReason;
    uint32_t outputCount = 0;  // Number of items produced by this stage
    uint64_t outputSize = 0;    // Size in bytes of output
};

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
    
    // Evidence-based stage validation
    StageValidation widgetStage;
    StageValidation layoutStage;
    StageValidation paintStage;
    StageValidation geometryStage;
    StageValidation uploadStage;
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
    
    // Evidence-based stage validation
    StageValidation pipelineBindStage;
    StageValidation bufferBindStage;
    StageValidation drawCallStage;
    StageValidation renderPassStage;
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
    
    // Evidence-based validation helpers
    static StageValidation ValidateWidgetStage(const std::shared_ptr<Widget>& root);
    static StageValidation ValidateLayoutStage(uint32_t widgetCount, uint32_t layoutPassCount);
    static StageValidation ValidatePaintStage(uint32_t paintCalls, uint32_t paintCommands);
    static StageValidation ValidateGeometryStage(uint32_t drawCommands, uint32_t vertexCount, uint32_t indexCount);
    static StageValidation ValidateUploadStage(bool geometryUploaded, VkBuffer vertexBuffer, VkBuffer indexBuffer);
    static StageValidation ValidatePipelineBindStage(VkPipeline pipeline);
    static StageValidation ValidateBufferBindStage(VkBuffer vertexBuffer, VkBuffer indexBuffer);
    static StageValidation ValidateDrawCallStage(uint32_t executedDrawCalls, uint32_t batchCount);
    static StageValidation ValidateRenderPassStage(VkCommandBuffer cmd, bool renderPassEnded);
};

} // namespace we::UI
