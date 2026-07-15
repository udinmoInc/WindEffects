#include "RHI/Result.h"
#include "RHI/Types.h"

namespace we::rhi {

const char* ToString(RHIErrorCode code) noexcept {
    switch (code) {
    case RHIErrorCode::Ok: return "Ok";
    case RHIErrorCode::NotInitialized: return "NotInitialized";
    case RHIErrorCode::AlreadyInitialized: return "AlreadyInitialized";
    case RHIErrorCode::NotSupported: return "NotSupported";
    case RHIErrorCode::InvalidArgument: return "InvalidArgument";
    case RHIErrorCode::InvalidHandle: return "InvalidHandle";
    case RHIErrorCode::OutOfMemory: return "OutOfMemory";
    case RHIErrorCode::DeviceLost: return "DeviceLost";
    case RHIErrorCode::OutOfDate: return "OutOfDate";
    case RHIErrorCode::Timeout: return "Timeout";
    case RHIErrorCode::BackendFailure: return "BackendFailure";
    case RHIErrorCode::WrongThread: return "WrongThread";
    case RHIErrorCode::Unknown: return "Unknown";
    }
    return "Unknown";
}

const char* ToString(RHIBackend backend) noexcept {
    switch (backend) {
    case RHIBackend::Auto: return "Auto";
    case RHIBackend::Null: return "Null";
    case RHIBackend::Vulkan: return "Vulkan";
    case RHIBackend::DirectX12: return "DirectX12";
    case RHIBackend::DirectX11: return "DirectX11";
    case RHIBackend::Metal: return "Metal";
    case RHIBackend::OpenGL: return "OpenGL";
    case RHIBackend::OpenGLES: return "OpenGLES";
    }
    return "Unknown";
}

} // namespace we::rhi
