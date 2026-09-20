// ==============================================================================
// WindEffects — TestViewport — Main
// Dedicated standalone application for fast, minimal cloud test development.
// Opens a native OS window with only the render viewport.
//
// NO editor UI, NO toolbar, NO inspector, NO hierarchy, NO HUD, NO terrain.
// 100% of client area is the rendering viewport.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// ==============================================================================
#include "CloudTestScene.h"

#include "Platform/PlatformSDK.h"
#include "Core/Logger.h"
#include "Core/BuildPaths.h"
#include "Core/ProductMetadata.h"
#include "Core/ExecutableMetadata.h"
#include "Renderer/Renderer.h"

#if defined(_WIN32)
#include "Platform/UndefWin32Macros.h"
#ifdef CreateWindow
#undef CreateWindow
#endif
#ifdef CreateWindowA
#undef CreateWindowA
#endif
#ifdef CreateWindowW
#undef CreateWindowW
#endif
#endif

#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <variant>

namespace {

int KeyCodeToChar(we::platform::KeyCode code)
{
    using we::platform::KeyCode;
    switch (code) {
    case KeyCode::W: return 'W';
    case KeyCode::A: return 'A';
    case KeyCode::S: return 'S';
    case KeyCode::D: return 'D';
    case KeyCode::Q: return 'Q';
    case KeyCode::E: return 'E';
    case KeyCode::LeftShift:
    case KeyCode::RightShift: return 0x10;
    case KeyCode::Num0: return '0';
    case KeyCode::Num1: return '1';
    case KeyCode::Num2: return '2';
    case KeyCode::Num3: return '3';
    case KeyCode::Num4: return '4';
    case KeyCode::Num5: return '5';
    case KeyCode::Num6: return '6';
    default: return 0;
    }
}

} // namespace

int main(int argc, char* argv[])
{
    try {
        we::runtime::core::Logger::Init();
        we::core::ProductMetadataService::Get().Initialize();

        we::platform::PlatformDesc platformDesc{};
        platformDesc.appName = "WindEffects Cloud Test Viewport";
        platformDesc.highDpiAware = true;
        platformDesc.enableRawInput = true;
        platformDesc.enableDiagnostics = true;

        auto& platform = we::platform::Platform::Initialize(platformDesc);

        const std::string exeDir = platform.GetExecutableDirectory();
        if (!exeDir.empty()) {
            platform.SetCurrentWorkingDirectory(exeDir);
        }
        we::core::ConfigureModuleSearchPaths();

        HE_INFO("[TestViewport] === Cloud Test Viewport starting ===");

        // Create native window: standard OS window frame with 100% client area rendering
        we::platform::WindowDesc windowDesc{};
        windowDesc.title = "WindEffects — Cloud Test Viewport (Press 0-6 for Debug Modes, WASD+RMB to Fly)";
        windowDesc.width = 1280;
        windowDesc.height = 720;
        windowDesc.resizable = true;
        windowDesc.borderless = false; // Normal OS window titlebar and borders
        windowDesc.visible = true;
        windowDesc.highDpi = true;

        const auto windowResult = platform.CreateWindow(windowDesc);
        if (!windowResult) {
            std::cerr << "[TestViewport] Failed to create native window: " << windowResult.error.message << std::endl;
            return 1;
        }
        const we::platform::WindowId window = *windowResult;
        HE_INFO("[TestViewport] Native window created (1280x720).");

        // Initialize engine renderer on the native window swapchain
        HE_INFO("[TestViewport] Initializing Renderer on native window swapchain...");
        auto renderer = std::make_unique<we::runtime::renderer::Renderer>();
        renderer->Init(window);

        // Initialize reusable minimal cloud test scene
        HE_INFO("[TestViewport] Initializing CloudTestScene...");
        we::test::CloudTestScene testScene(renderer.get());
        testScene.Init(
            renderer->GetSwapchainWidth() > 0 ? renderer->GetSwapchainWidth() : windowDesc.width,
            renderer->GetSwapchainHeight() > 0 ? renderer->GetSwapchainHeight() : windowDesc.height);

        HE_INFO("[TestViewport] Cloud test scene ready. Starting main render loop.");
        HE_INFO("[TestViewport] Controls: Right-Click + Mouse = Look, W/A/S/D/Q/E = Fly, Keys 0-6 = Formation Debug Modes");

        uint64_t lastTime = platform.GetHighResolutionCounter();
        const double frequency = static_cast<double>(platform.GetHighResolutionFrequency());
        bool running = true;

        uint32_t frameCounter = 0;
        double fpsTimer = 0.0;

        while (running) {
            // Process OS platform events
            if (!platform.PollEvents()) {
                running = false;
                break;
            }

            for (const auto& event : platform.GetFrameEvents()) {
                if (std::holds_alternative<we::platform::QuitEvent>(event)) {
                    running = false;
                }
                else if (const auto* close = std::get_if<we::platform::WindowCloseEvent>(&event)) {
                    if (close->window == window) {
                        running = false;
                    }
                }
                else if (const auto* resize = std::get_if<we::platform::WindowResizeEvent>(&event)) {
                    if (resize->window == window && resize->pixelSize.x > 0 && resize->pixelSize.y > 0) {
                        renderer->RecreateSwapchain(resize->pixelSize.x, resize->pixelSize.y);
                        testScene.Resize(resize->pixelSize.x, resize->pixelSize.y);
                    }
                }
                else if (const auto* move = std::get_if<we::platform::MouseMoveEvent>(&event)) {
                    testScene.OnMouseMove(
                        static_cast<float>(move->position.x),
                        static_cast<float>(move->position.y));
                }
                else if (const auto* button = std::get_if<we::platform::MouseButtonEvent>(&event)) {
                    testScene.OnMouseButton(static_cast<int>(button->button), button->pressed);
                }
                else if (const auto* wheel = std::get_if<we::platform::MouseWheelEvent>(&event)) {
                    testScene.OnMouseWheel(wheel->delta.y);
                }
                else if (const auto* key = std::get_if<we::platform::KeyEvent>(&event)) {
                    if (key->key == we::platform::KeyCode::Escape && key->pressed) {
                        running = false;
                    } else {
                        const int ch = KeyCodeToChar(key->key);
                        if (ch != 0) {
                            testScene.OnKeyDown(ch, key->pressed);
                        }
                    }
                }
            }

            if (!running) {
                break;
            }

            // Calculate delta time
            const uint64_t now = platform.GetHighResolutionCounter();
            float dt = static_cast<float>((now - lastTime) / frequency);
            lastTime = now;
            if (dt > 0.1f) {
                dt = 0.1f;
            }

            // FPS reporting
            ++frameCounter;
            fpsTimer += dt;
            if (fpsTimer >= 2.0) {
                const double fps = static_cast<double>(frameCounter) / fpsTimer;
                HE_INFO("[TestViewport] Viewport running: " + std::to_string(static_cast<int>(fps)) + " FPS");
                frameCounter = 0;
                fpsTimer = 0.0;
            }

            // Update scene and camera simulation
            testScene.Update(dt);

            // Render frame directly through engine RenderGraph to the native swapchain
            if (renderer->BeginFrame()) {
                testScene.Render();
                renderer->SubmitFrame();
                renderer->PresentFrame();
            }
        }

        HE_INFO("[TestViewport] Shutting down...");
        renderer->Shutdown();
        renderer.reset();

        (void)platform.DestroyWindow(window);
        we::platform::Platform::Shutdown();

        HE_INFO("[TestViewport] === Cloud Test Viewport closed cleanly ===");
    }
    catch (const std::exception& e) {
        std::cerr << "[TestViewport] Fatal exception: " << e.what() << std::endl;
        HE_ERROR(std::string("[TestViewport] Fatal exception: ") + e.what());
        return -1;
    }

    return 0;
}
