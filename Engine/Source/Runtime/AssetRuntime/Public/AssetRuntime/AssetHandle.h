#pragma once

#include "AssetImporter/AssetGuid.h"
#include "AssetRuntime/Export.h"

#include <cstdint>
#include <functional>

namespace we::runtime::assetruntime {

/// Generational handle into the Runtime Asset Manager slot table.
/// Remains valid across streaming; invalid after Unload / slot reuse.
struct ASSETRUNTIME_API AssetHandle {
    uint32_t index = 0;
    uint32_t generation = 0;

    [[nodiscard]] static constexpr AssetHandle Invalid() noexcept { return {}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return index != 0; }

    [[nodiscard]] constexpr bool operator==(const AssetHandle& other) const noexcept {
        return index == other.index && generation == other.generation;
    }
    [[nodiscard]] constexpr bool operator!=(const AssetHandle& other) const noexcept {
        return !(*this == other);
    }
};

/// Non-owning observe handle. Lock() fails once the strong refcount reaches zero and the
/// asset is unloaded / slot recycled.
struct ASSETRUNTIME_API WeakAssetHandle {
    AssetHandle handle{};
    uint32_t weakEpoch = 0;

    [[nodiscard]] static constexpr WeakAssetHandle Invalid() noexcept { return {}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return handle.IsValid(); }

    [[nodiscard]] constexpr bool operator==(const WeakAssetHandle& other) const noexcept {
        return handle == other.handle && weakEpoch == other.weakEpoch;
    }
    [[nodiscard]] constexpr bool operator!=(const WeakAssetHandle& other) const noexcept {
        return !(*this == other);
    }
};

/// Opaque id for an in-flight stream / async load request (cancellable).
struct ASSETRUNTIME_API StreamRequestId {
    uint64_t value = 0;

    [[nodiscard]] static constexpr StreamRequestId Invalid() noexcept { return {}; }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }

    [[nodiscard]] constexpr bool operator==(const StreamRequestId& other) const noexcept {
        return value == other.value;
    }
    [[nodiscard]] constexpr bool operator!=(const StreamRequestId& other) const noexcept {
        return !(*this == other);
    }
};

struct ASSETRUNTIME_API AssetHandleHash {
    [[nodiscard]] std::size_t operator()(const AssetHandle& h) const noexcept {
        return (static_cast<std::size_t>(h.index) << 32) ^ static_cast<std::size_t>(h.generation);
    }
};

struct ASSETRUNTIME_API AssetGuidOrHandle {
    we::runtime::assetimporter::AssetGuid guid{};
    AssetHandle handle{};

    [[nodiscard]] bool HasGuid() const noexcept { return !guid.IsNil(); }
    [[nodiscard]] bool HasHandle() const noexcept { return handle.IsValid(); }
};

} // namespace we::runtime::assetruntime
