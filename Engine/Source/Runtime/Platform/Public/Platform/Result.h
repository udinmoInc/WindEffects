#pragma once

#include "Platform/Export.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251) // STL members in exported types
#endif

namespace we::platform {

// Structured platform errors — never silent failures for ownership/API failures.
enum class PlatformErrorCode : uint32_t {
    Ok = 0,
    NotInitialized,
    AlreadyInitialized,
    NotSupported,
    InvalidArgument,
    InvalidHandle,
    OutOfMemory,
    AccessDenied,
    NotFound,
    Busy,
    Timeout,
    OsFailure,
    WrongThread,
    ServiceDisabled,
    Cancelled,
    Unknown
};

struct PLATFORM_API PlatformError {
    PlatformErrorCode code = PlatformErrorCode::Ok;
    int32_t osCode = 0;              // OS-native error (GetLastError / errno) when applicable
    const char* api = nullptr;       // Platform API name that failed
    std::string message;

    [[nodiscard]] constexpr bool Ok() const noexcept { return code == PlatformErrorCode::Ok; }
    [[nodiscard]] explicit operator bool() const noexcept { return Ok(); }

    [[nodiscard]] static PlatformError Success() noexcept {
        return {};
    }

    [[nodiscard]] static PlatformError Make(
        PlatformErrorCode code,
        std::string_view message,
        const char* api = nullptr,
        int32_t osCode = 0)
    {
        PlatformError err;
        err.code = code;
        err.message = std::string(message);
        err.api = api;
        err.osCode = osCode;
        return err;
    }
};

[[nodiscard]] PLATFORM_API const char* ToString(PlatformErrorCode code) noexcept;

// Lightweight Result<T> — value valid only when error.Ok().
template <typename T>
struct Result {
    T value{};
    PlatformError error{};

    Result() = default;
    Result(T v) : value(std::move(v)), error(PlatformError::Success()) {}
    Result(PlatformError e) : error(std::move(e)) {}

    [[nodiscard]] bool Ok() const noexcept { return error.Ok(); }
    [[nodiscard]] explicit operator bool() const noexcept { return Ok(); }

    [[nodiscard]] T& operator*() & { return value; }
    [[nodiscard]] const T& operator*() const& { return value; }
    [[nodiscard]] T* operator->() { return &value; }
    [[nodiscard]] const T* operator->() const { return &value; }
};

template <>
struct Result<void> {
    PlatformError error{};

    Result() = default;
    Result(PlatformError e) : error(std::move(e)) {}

    [[nodiscard]] static Result Success() { return Result{PlatformError::Success()}; }

    [[nodiscard]] bool Ok() const noexcept { return error.Ok(); }
    [[nodiscard]] explicit operator bool() const noexcept { return Ok(); }
};

} // namespace we::platform

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
