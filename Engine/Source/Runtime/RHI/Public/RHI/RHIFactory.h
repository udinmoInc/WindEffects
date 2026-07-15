#pragma once

#include "RHI/Export.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <functional>
#include <memory>

namespace we::rhi {

using RHIBackendCreateFn = std::unique_ptr<IRHI> (*)();

class RHI_API RHIFactory {
public:
    static void Register(RHIBackend backend, RHIBackendCreateFn createFn, const char* name);
    [[nodiscard]] static std::unique_ptr<IRHI> Create(RHIBackend preferred = RHIBackend::Auto);
    [[nodiscard]] static const char* GetRegisteredName(RHIBackend backend);
    [[nodiscard]] static bool IsRegistered(RHIBackend backend);
};

// Convenience registration helper for backend modules.
struct RHI_API RHIBackendRegistrar {
    RHIBackendRegistrar(RHIBackend backend, RHIBackendCreateFn fn, const char* name) {
        RHIFactory::Register(backend, fn, name);
    }
};

#define WE_REGISTER_RHI_BACKEND(BackendEnum, CreateFn, NameLiteral) \
    static ::we::rhi::RHIBackendRegistrar WE_CONCAT_RHI_REG(_reg_, __LINE__)( \
        ::we::rhi::RHIBackend::BackendEnum, CreateFn, NameLiteral)

#define WE_CONCAT_RHI_REG_INNER(a, b) a##b
#define WE_CONCAT_RHI_REG(a, b) WE_CONCAT_RHI_REG_INNER(a, b)

} // namespace we::rhi
