#pragma once

#include "Renderer/Renderer.h"
#include "Renderer/ViewportInterfaces.h"
#include "Camera/CameraUniform.h"
#include "Resource/DepthTarget.h"
#include "Platform/Types.h"
#include "RHI/RHISDK.h"

namespace we::runtime::renderer {

struct RendererInitOptions {
    we::platform::WindowId window = we::platform::WindowId::Invalid;
};

inline void InitializeRenderer(Renderer& renderer, const RendererInitOptions& options) {
    renderer.Init(options.window);
}

} // namespace we::runtime::renderer
