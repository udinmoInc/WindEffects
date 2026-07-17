#pragma once

#include "KindUI/Export.h"
#include "RHI/IRHI.h"
#include "KindUI/Core/Widget.h"

#include <cstdint>
#include <string>

namespace we::runtime::kindui {

struct StageValidation {
    bool executed = false;
    bool succeeded = false;
    std::string failureReason;
    uint32_t outputCount = 0;
    uint64_t outputSize = 0;
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
    StageValidation widgetStage;
    StageValidation layoutStage;
    StageValidation paintStage;
    StageValidation geometryStage;
    StageValidation uploadStage;
};

struct UIPipelineGpuStats {
    bool commandListValid = false;
    bool pipelineBound = false;
    bool vertexBufferBound = false;
    bool indexBufferBound = false;
    uint32_t drawCalls = 0;
};

class KINDUI_API UIPipelineAudit {
public:
    static StageValidation ValidateRenderPassStage(we::rhi::IRHICommandList* cmd, bool renderPassEnded);
};

} // namespace we::runtime::kindui
