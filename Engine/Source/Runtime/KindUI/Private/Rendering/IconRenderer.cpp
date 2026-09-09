// ==============================================================================
// WindEffects — KindUI — IconRenderer
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Rendering/IconRenderer.h"

namespace we::runtime::kindui {

IconRenderer::IconRenderer() = default;

IconRenderer::~IconRenderer() {
    Shutdown();
}

bool IconRenderer::Init(we::rhi::IRHIDevice* device, UiGpuUpload* gpuUpload) {
    m_Device = device;
    m_GpuUpload = gpuUpload;
    return true;
}

void IconRenderer::Shutdown() {
    ClearCache();
    m_Device = nullptr;
    m_GpuUpload = nullptr;
}

IconDrawInfo IconRenderer::GetIconDrawInfo(WindIconRef icon) const {
    if (!m_IconManager) {
        return {};
    }
    return m_IconManager->ResolveIcon(icon);
}

we::rhi::RHIDescriptorSetHandle IconRenderer::CreateTextureFromBitmap(
    const std::vector<uint8_t>& bitmap,
    const uint32_t width,
    const uint32_t height) {
    (void)bitmap;
    (void)width;
    (void)height;
    return we::rhi::RHIDescriptorSetHandle::Invalid;
}

void IconRenderer::ClearCache() {
    m_ThumbnailCache.clear();
}

} // namespace we::runtime::kindui
 
