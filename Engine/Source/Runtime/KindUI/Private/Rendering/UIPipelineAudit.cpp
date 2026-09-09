// ==============================================================================
// WindEffects — KindUI — UIPipelineAudit
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
 
