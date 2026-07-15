#pragma once

#include "RHI/Desc.h"
#include "RHI/Export.h"
#include "RHI/IRHI.h"

namespace we::rhi {

// Facade mirroring Platform::Get() — auto-selects and initializes the active backend.
class RHI_API RHI {
public:
    [[nodiscard]] static IRHI& Initialize(const RHIInitDesc& desc = {});
    [[nodiscard]] static IRHI& Get();
    [[nodiscard]] static bool IsAvailable() noexcept;
    static void Shutdown();

    RHI() = delete;
};

} // namespace we::rhi
