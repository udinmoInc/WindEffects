#pragma once

// WindEffects Renderer SDK — rendering, shaders, and viewport control.
// Prefer this header over including Renderer internals directly.

#include "WindEffects/Platform.h"

#include "Renderer/Renderer.h"
#include "Renderer/ViewportInterfaces.h"
#include "Graph/RenderGraph.h"
#include "Scene/SceneRenderer.h"
#include "Shader/ShaderLibrary.h"
#include "Core/SwapchainManager.h"
#include "Core/DeviceContext.h"
#include "Resource/ResourceManager.h"

namespace we::runtime::renderer {

struct RendererInitOptions {
    SDL_Window* window = nullptr;
};

inline void InitializeRenderer(Renderer& renderer, const RendererInitOptions& options) {
    renderer.Init(options.window);
}

} // namespace we::runtime::renderer
