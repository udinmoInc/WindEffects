#pragma once

#include "RHI/Export.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

namespace we::rhi {

enum class RHIErrorCode : uint32_t {
    Ok = 0,
    NotInitialized,
    AlreadyInitialized,
    NotSupported,
    InvalidArgument,
    InvalidHandle,
    OutOfMemory,
    DeviceLost,
    OutOfDate,
    Timeout,
    BackendFailure,
    WrongThread,
    Unknown
};

struct RHI_API RHIError {
    RHIErrorCode code = RHIErrorCode::Ok;
    int32_t backendCode = 0;
    const char* api = nullptr;
    std::string message;

    [[nodiscard]] constexpr bool Ok() const noexcept { return code == RHIErrorCode::Ok; }
    [[nodiscard]] explicit operator bool() const noexcept { return Ok(); }

    [[nodiscard]] static RHIError Success() noexcept { return {}; }

    [[nodiscard]] static RHIError Make(
        RHIErrorCode code,
        std::string_view message,
        const char* api = nullptr,
        int32_t backendCode = 0)
    {
        RHIError err;
        err.code = code;
        err.message = std::string(message);
        err.api = api;
        err.backendCode = backendCode;
        return err;
    }
};

[[nodiscard]] RHI_API const char* ToString(RHIErrorCode code) noexcept;

template <typename T>
struct RHIResult {
    T value{};
    RHIError error{};

    RHIResult() = default;
    RHIResult(T v) : value(std::move(v)), error(RHIError::Success()) {}
    RHIResult(RHIError e) : error(std::move(e)) {}

    [[nodiscard]] bool Ok() const noexcept { return error.Ok(); }
    [[nodiscard]] explicit operator bool() const noexcept { return Ok(); }
    [[nodiscard]] T& operator*() & { return value; }
    [[nodiscard]] const T& operator*() const& { return value; }
    [[nodiscard]] T* operator->() { return &value; }
    [[nodiscard]] const T* operator->() const { return &value; }
};

template <>
struct RHIResult<void> {
    RHIError error{};
    RHIResult() = default;
    RHIResult(RHIError e) : error(std::move(e)) {}
    [[nodiscard]] static RHIResult Success() { return RHIResult{RHIError::Success()}; }
    [[nodiscard]] bool Ok() const noexcept { return error.Ok(); }
    [[nodiscard]] explicit operator bool() const noexcept { return Ok(); }
};

} // namespace we::rhi

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
