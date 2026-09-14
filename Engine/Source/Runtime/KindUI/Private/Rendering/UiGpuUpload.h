// ==============================================================================
// WindEffects — KindUI — UiGpuUpload
// Shared geometry/texture upload helpers for the KindUI GPU path.
// Owned by OverlayRenderer; consumed by UiImmediateRenderer (canonical uploader).
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Diagnostics/UiGpuPathStats.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <cstdint>
#include <span>

namespace we::runtime::kindui {

/// FNV-1a over raw bytes (geometry content identity for skip-upload).
[[nodiscard]] inline uint64_t HashGpuBytes(std::span<const uint8_t> bytes) noexcept {
    uint64_t hash = 14695981039346656037ull;
    for (uint8_t b : bytes) {
        hash ^= static_cast<uint64_t>(b);
        hash *= 1099511628211ull;
    }
    return hash;
}

[[nodiscard]] inline uint64_t HashGpuBytesCombine(uint64_t a, uint64_t b) noexcept {
    uint64_t hash = a;
    hash ^= b + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
    return hash;
}

/// Device handle holder kept for IconRenderer / Overlay compatibility.
/// All real GPU buffer/texture work lives in UiImmediateRenderer.
class UiGpuUpload {
public:
    void Init(we::rhi::IRHIDevice* device);
    void Shutdown();

    [[nodiscard]] we::rhi::IRHIDevice* GetDevice() const { return m_Device; }

private:
    we::rhi::IRHIDevice* m_Device = nullptr;
};

} // namespace we::runtime::kindui
