#pragma once

#include "RHI/IRHI.h"

#include <memory>

namespace we::rhi {

// Shared stub IRHI used by DX12/DX11/Metal/GL/GLES modules until implemented.
[[nodiscard]] RHI_API std::unique_ptr<IRHI> CreateUnsupportedRHI(RHIBackend backend, const char* name);

} // namespace we::rhi
