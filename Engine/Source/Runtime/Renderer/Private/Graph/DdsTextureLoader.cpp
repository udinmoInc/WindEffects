// ==============================================================================
// WindEffects — Renderer — DdsTextureLoader
// Internal implementation for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Graph/DdsTextureLoader.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "RHI/Desc.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <span>

namespace we::runtime::renderer {
namespace {

constexpr uint32_t kDdsMagic = 0x20534444u; // "DDS "
constexpr uint32_t kDdsHeaderSize = 124;
constexpr uint32_t kDdpfFourCc = 0x4;
constexpr uint32_t kDxgiR8G8B8A8Unorm = 28;
constexpr uint32_t kResourceDimensionTexture2D = 3;
constexpr uint32_t kResourceDimensionTexture3D = 4;

uint32_t FourCC(const char* tag) {
    return static_cast<uint32_t>(static_cast<uint8_t>(tag[0]))
        | (static_cast<uint32_t>(static_cast<uint8_t>(tag[1])) << 8)
        | (static_cast<uint32_t>(static_cast<uint8_t>(tag[2])) << 16)
        | (static_cast<uint32_t>(static_cast<uint8_t>(tag[3])) << 24);
}

[[nodiscard]] uint32_t CalcMipCount(uint32_t width, uint32_t height, uint32_t depth) {
    uint32_t levels = 1;
    while (width > 1 || height > 1 || depth > 1) {
        width = (std::max)(1u, width / 2u);
        height = (std::max)(1u, height / 2u);
        depth = (std::max)(1u, depth / 2u);
        ++levels;
    }
    return levels;
}

// Box-filter downsample for RGBA8 volumes / 2D (depth==1). Supports wrap-periodic
// noise so WRAP sampling stays seamless across mip boundaries.
void DownsampleRgba8Box(
    const uint8_t* src,
    uint32_t srcW,
    uint32_t srcH,
    uint32_t srcD,
    std::vector<uint8_t>& dst,
    uint32_t dstW,
    uint32_t dstH,
    uint32_t dstD)
{
    dst.assign(static_cast<size_t>(dstW) * dstH * dstD * 4ull, 0);
    for (uint32_t z = 0; z < dstD; ++z) {
        for (uint32_t y = 0; y < dstH; ++y) {
            for (uint32_t x = 0; x < dstW; ++x) {
                uint32_t acc[4] = {0, 0, 0, 0};
                uint32_t samples = 0;
                for (uint32_t dz = 0; dz < 2; ++dz) {
                    for (uint32_t dy = 0; dy < 2; ++dy) {
                        for (uint32_t dx = 0; dx < 2; ++dx) {
                            const uint32_t sx = (std::min)(x * 2u + dx, srcW - 1u);
                            const uint32_t sy = (std::min)(y * 2u + dy, srcH - 1u);
                            const uint32_t sz = (std::min)(z * 2u + dz, srcD - 1u);
                            const size_t si =
                                (static_cast<size_t>(sz) * srcH + sy) * srcW + sx;
                            const uint8_t* p = src + si * 4ull;
                            acc[0] += p[0];
                            acc[1] += p[1];
                            acc[2] += p[2];
                            acc[3] += p[3];
                            ++samples;
                        }
                    }
                }
                const size_t di = (static_cast<size_t>(z) * dstH + y) * dstW + x;
                uint8_t* out = dst.data() + di * 4ull;
                out[0] = static_cast<uint8_t>((acc[0] + samples / 2u) / samples);
                out[1] = static_cast<uint8_t>((acc[1] + samples / 2u) / samples);
                out[2] = static_cast<uint8_t>((acc[2] + samples / 2u) / samples);
                out[3] = static_cast<uint8_t>((acc[3] + samples / 2u) / samples);
            }
        }
    }
}

} // namespace

std::optional<LoadedDdsTexture> LoadDdsRgba8Texture(
    we::rhi::IRHIDevice& device,
    const std::filesystem::path& path,
    const char* debugName)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            std::string("DDS load failed (open): ") + path.string());
        return std::nullopt;
    }

    uint32_t magic = 0;
    file.read(reinterpret_cast<char*>(&magic), 4);
    if (!file || magic != kDdsMagic) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            std::string("DDS load failed (magic): ") + path.string());
        return std::nullopt;
    }

    uint8_t header[kDdsHeaderSize]{};
    file.read(reinterpret_cast<char*>(header), kDdsHeaderSize);
    if (!file) {
        return std::nullopt;
    }

    const uint32_t dwSize = *reinterpret_cast<const uint32_t*>(header + 0);
    const uint32_t height = *reinterpret_cast<const uint32_t*>(header + 8);
    const uint32_t width = *reinterpret_cast<const uint32_t*>(header + 12);
    const uint32_t depth = *reinterpret_cast<const uint32_t*>(header + 20);
    const uint32_t pfFlags = *reinterpret_cast<const uint32_t*>(header + 76);
    const uint32_t fourCC = *reinterpret_cast<const uint32_t*>(header + 80);

    if (dwSize != kDdsHeaderSize || width == 0 || height == 0) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            std::string("DDS load failed (header): ") + path.string());
        return std::nullopt;
    }
    if ((pfFlags & kDdpfFourCc) == 0 || fourCC != FourCC("DX10")) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            std::string("DDS load failed (expected DX10): ") + path.string());
        return std::nullopt;
    }

    uint8_t dx10[20]{};
    file.read(reinterpret_cast<char*>(dx10), 20);
    if (!file) {
        return std::nullopt;
    }

    const uint32_t dxgiFormat = *reinterpret_cast<const uint32_t*>(dx10 + 0);
    const uint32_t dimension = *reinterpret_cast<const uint32_t*>(dx10 + 4);
    if (dxgiFormat != kDxgiR8G8B8A8Unorm) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            std::string("DDS load failed (format): ") + path.string());
        return std::nullopt;
    }

    const bool isVolume = dimension == kResourceDimensionTexture3D || depth > 1;
    if (!isVolume && dimension != kResourceDimensionTexture2D) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            std::string("DDS load failed (dimension): ") + path.string());
        return std::nullopt;
    }

    const uint32_t sliceDepth = isVolume ? (depth > 0 ? depth : 1u) : 1u;
    const uint64_t payloadBytes =
        static_cast<uint64_t>(width) * height * sliceDepth * 4ull;
    std::vector<uint8_t> pixels(static_cast<size_t>(payloadBytes));
    file.read(reinterpret_cast<char*>(pixels.data()), static_cast<std::streamsize>(payloadBytes));
    if (!file) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            std::string("DDS load failed (payload): ") + path.string());
        return std::nullopt;
    }

    // Full mip chain so linear/trilinear filtering is valid when LOD is introduced later.
    // Runtime still samples mip 0 for shape diagnostics.
    // NOTE: some drivers mis-handle multi-mip 3D UpdateTexture during early device
    // lifetime — keep volumes on mip0-only for reliability; 2D can use a short chain.
    const uint32_t mipLevels = isVolume
        ? 1u
        : (std::min)(4u, CalcMipCount(width, height, 1u));

    we::rhi::TextureDesc texDesc{};
    texDesc.extent = {width, height, sliceDepth};
    texDesc.format = we::rhi::Format::R8G8B8A8_UNORM;
    texDesc.usage = we::rhi::TextureUsage::Sampled | we::rhi::TextureUsage::TransferDst;
    texDesc.mipLevels = mipLevels;
    texDesc.arrayLayers = 1;
    texDesc.debugName = debugName;
    auto texture = device.CreateTexture(texDesc);
    if (!texture) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(),
            std::string("DDS CreateTexture failed: ") + path.string());
        return std::nullopt;
    }

    auto uploadMip = [&](uint32_t mip, uint32_t w, uint32_t h, uint32_t d,
                         const std::vector<uint8_t>& src) -> bool {
        const uint32_t rowPitch = w * 4u;
        const uint64_t sliceBytes = static_cast<uint64_t>(rowPitch) * h;
        if (isVolume) {
            for (uint32_t z = 0; z < d; ++z) {
                we::rhi::TextureUpdateDesc update{};
                update.mipLevel = mip;
                update.offsetZ = z;
                update.extent = {w, h, 1};
                update.rowPitch = rowPitch;
                const size_t offset = static_cast<size_t>(z) * static_cast<size_t>(sliceBytes);
                update.data = std::span<const uint8_t>(
                    src.data() + offset, static_cast<size_t>(sliceBytes));
                if (!device.UpdateTexture(*texture, update)) {
                    WE_LOG_ERROR(we::LogCategory::Renderer.data(),
                        std::string("DDS UpdateTexture mip=") + std::to_string(mip)
                            + " z=" + std::to_string(z) + " " + path.string());
                    return false;
                }
            }
            return true;
        }

        we::rhi::TextureUpdateDesc update{};
        update.mipLevel = mip;
        update.extent = {w, h, 1};
        update.data = src;
        update.rowPitch = rowPitch;
        if (!device.UpdateTexture(*texture, update)) {
            WE_LOG_ERROR(we::LogCategory::Renderer.data(),
                std::string("DDS UpdateTexture mip=") + std::to_string(mip)
                    + " " + path.string());
            return false;
        }
        return true;
    };

    if (!uploadMip(0, width, height, sliceDepth, pixels)) {
        (void)device.DestroyTexture(*texture);
        return std::nullopt;
    }

    std::vector<uint8_t> srcMip = pixels;
    uint32_t mipW = width;
    uint32_t mipH = height;
    uint32_t mipD = sliceDepth;
    for (uint32_t mip = 1; mip < mipLevels; ++mip) {
        const uint32_t nextW = (std::max)(1u, mipW / 2u);
        const uint32_t nextH = (std::max)(1u, mipH / 2u);
        const uint32_t nextD = isVolume ? (std::max)(1u, mipD / 2u) : 1u;
        std::vector<uint8_t> nextMip;
        DownsampleRgba8Box(srcMip.data(), mipW, mipH, mipD, nextMip, nextW, nextH, nextD);
        if (!uploadMip(mip, nextW, nextH, nextD, nextMip)) {
            (void)device.DestroyTexture(*texture);
            return std::nullopt;
        }
        srcMip.swap(nextMip);
        mipW = nextW;
        mipH = nextH;
        mipD = nextD;
    }

    we::rhi::TextureViewDesc viewDesc{};
    viewDesc.texture = *texture;
    viewDesc.format = we::rhi::Format::R8G8B8A8_UNORM;
    viewDesc.baseMip = 0;
    viewDesc.mipCount = mipLevels;
    viewDesc.debugName = debugName;
    auto view = device.CreateTextureView(viewDesc);
    if (!view) {
        (void)device.DestroyTexture(*texture);
        return std::nullopt;
    }

    LoadedDdsTexture out{};
    out.texture = *texture;
    out.view = *view;
    out.width = width;
    out.height = height;
    out.depth = sliceDepth;
    out.mipLevels = mipLevels;
    out.isVolume = isVolume;

    WE_LOG_INFO(we::LogCategory::Renderer.data(),
        std::string("Loaded DDS ") + path.filename().string()
            + " " + std::to_string(width) + "x" + std::to_string(height)
            + (isVolume ? ("x" + std::to_string(sliceDepth)) : "")
            + " mips=" + std::to_string(mipLevels)
            + " (" + debugName + ")");
    return out;
}

void DestroyLoadedDdsTexture(we::rhi::IRHIDevice& device, LoadedDdsTexture& tex) {
    if (tex.view != we::rhi::RHITextureViewHandle::Invalid) {
        (void)device.DestroyTextureView(tex.view);
        tex.view = we::rhi::RHITextureViewHandle::Invalid;
    }
    if (tex.texture != we::rhi::RHITextureHandle::Invalid) {
        (void)device.DestroyTexture(tex.texture);
        tex.texture = we::rhi::RHITextureHandle::Invalid;
    }
}

} // namespace we::runtime::renderer
