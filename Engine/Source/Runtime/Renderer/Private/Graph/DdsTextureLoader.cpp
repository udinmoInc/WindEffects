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

    we::rhi::TextureDesc texDesc{};
    texDesc.extent = {width, height, sliceDepth};
    texDesc.format = we::rhi::Format::R8G8B8A8_UNORM;
    texDesc.usage = we::rhi::TextureUsage::Sampled | we::rhi::TextureUsage::TransferDst;
    texDesc.mipLevels = 1;
    texDesc.arrayLayers = 1;
    texDesc.debugName = debugName;
    auto texture = device.CreateTexture(texDesc);
    if (!texture) {
        WE_LOG_ERROR(we::LogCategory::Renderer.data(),
            std::string("DDS CreateTexture failed: ") + path.string());
        return std::nullopt;
    }

    // Upload volume textures one depth-slice at a time. A single 128^3 copy has
    // crashed some Vulkan drivers during early device lifetime.
    const uint32_t rowPitch = width * 4u;
    const uint64_t sliceBytes = static_cast<uint64_t>(rowPitch) * height;
    bool uploadOk = true;
    if (isVolume) {
        for (uint32_t z = 0; z < sliceDepth; ++z) {
            we::rhi::TextureUpdateDesc update{};
            update.offsetZ = z;
            update.extent = {width, height, 1};
            update.rowPitch = rowPitch;
            const size_t offset = static_cast<size_t>(z) * static_cast<size_t>(sliceBytes);
            update.data = std::span<const uint8_t>(
                pixels.data() + offset, static_cast<size_t>(sliceBytes));
            if (!device.UpdateTexture(*texture, update)) {
                uploadOk = false;
                WE_LOG_ERROR(we::LogCategory::Renderer.data(),
                    std::string("DDS UpdateTexture slice failed z=") + std::to_string(z)
                        + " " + path.string());
                break;
            }
        }
    } else {
        we::rhi::TextureUpdateDesc update{};
        update.extent = {width, height, 1};
        update.data = pixels;
        update.rowPitch = rowPitch;
        uploadOk = static_cast<bool>(device.UpdateTexture(*texture, update));
        if (!uploadOk) {
            WE_LOG_ERROR(we::LogCategory::Renderer.data(),
                std::string("DDS UpdateTexture failed: ") + path.string());
        }
    }
    if (!uploadOk) {
        (void)device.DestroyTexture(*texture);
        return std::nullopt;
    }

    we::rhi::TextureViewDesc viewDesc{};
    viewDesc.texture = *texture;
    viewDesc.format = we::rhi::Format::R8G8B8A8_UNORM;
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
    out.isVolume = isVolume;

    WE_LOG_INFO(we::LogCategory::Renderer.data(),
        std::string("Loaded DDS ") + path.filename().string()
            + " " + std::to_string(width) + "x" + std::to_string(height)
            + (isVolume ? ("x" + std::to_string(sliceDepth)) : "")
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
