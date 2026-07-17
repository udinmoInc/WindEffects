#include "KindUI/Rendering/UIPipelineAudit.h"

namespace we::runtime::kindui {

StageValidation UIPipelineAudit::ValidateRenderPassStage(we::rhi::IRHICommandList* cmd, bool renderPassEnded) {
    StageValidation result;
    result.executed = true;
    result.succeeded = cmd != nullptr && renderPassEnded;
    if (!result.succeeded) {
        result.failureReason = cmd ? "Render pass not ended" : "Command list invalid";
    }
    return result;
}

} // namespace we::runtime::kindui
