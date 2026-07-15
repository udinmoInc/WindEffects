#include "Platform/Result.h"

namespace we::platform {

const char* ToString(PlatformErrorCode code) noexcept {
    switch (code) {
    case PlatformErrorCode::Ok: return "Ok";
    case PlatformErrorCode::NotInitialized: return "NotInitialized";
    case PlatformErrorCode::AlreadyInitialized: return "AlreadyInitialized";
    case PlatformErrorCode::NotSupported: return "NotSupported";
    case PlatformErrorCode::InvalidArgument: return "InvalidArgument";
    case PlatformErrorCode::InvalidHandle: return "InvalidHandle";
    case PlatformErrorCode::OutOfMemory: return "OutOfMemory";
    case PlatformErrorCode::AccessDenied: return "AccessDenied";
    case PlatformErrorCode::NotFound: return "NotFound";
    case PlatformErrorCode::Busy: return "Busy";
    case PlatformErrorCode::Timeout: return "Timeout";
    case PlatformErrorCode::OsFailure: return "OsFailure";
    case PlatformErrorCode::WrongThread: return "WrongThread";
    case PlatformErrorCode::ServiceDisabled: return "ServiceDisabled";
    case PlatformErrorCode::Cancelled: return "Cancelled";
    case PlatformErrorCode::Unknown: return "Unknown";
    }
    return "Unknown";
}

} // namespace we::platform
