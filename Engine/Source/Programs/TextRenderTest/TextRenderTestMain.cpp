#include "Text/Rendering/TextGpuBackend.h"
#include "Text/TextEngine.h"

#include <iostream>

int main()
{
    using namespace we::runtime::text;

    auto engine = CreateTextEngine();
    auto vulkanBackend = rendering::CreateTextGpuBackend(GraphicsApi::Vulkan);
    auto dx12Backend = rendering::CreateTextGpuBackend(GraphicsApi::DirectX12);
    auto metalBackend = rendering::CreateTextGpuBackend(GraphicsApi::Metal);
    auto glBackend = rendering::CreateTextGpuBackend(GraphicsApi::OpenGL);

    rendering::GpuBackendConfig config;
    vulkanBackend->Initialize(config);
    dx12Backend->Initialize(config);
    metalBackend->Initialize(config);
    glBackend->Initialize(config);

    layout::TextStyle style;
    style.family = "Inter";
    style.sizePx = 18.0f;

    layout::LayoutConstraints constraints;
    constraints.maxWidth = 640.0f;

    const auto layout = engine->Layout("Hello, WindEffects text engine.", style, constraints);
    std::cout << "Layout glyphs: " << layout.glyphs.size() << " width: " << layout.bounds.width << '\n';

    const layout::LayoutResult layouts[] = {layout};
    engine->SubmitFrame(*vulkanBackend, layouts);
    engine->SubmitFrame(*dx12Backend, layouts);
    engine->SubmitFrame(*metalBackend, layouts);
    engine->SubmitFrame(*glBackend, layouts);

    const auto snapshot = engine->Diagnostics().CaptureSnapshot();
    std::cout << "Diagnostics messages: " << snapshot.messages.size() << '\n';
    return 0;
}
