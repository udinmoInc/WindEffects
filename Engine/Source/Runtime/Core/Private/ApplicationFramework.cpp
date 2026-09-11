// ==============================================================================
// WindEffects — Core — ApplicationFramework
// Decoupled application framework for engine applications (Editor and Runtime)
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Core/ApplicationFramework.h"
#include "Core/Logger.h"
#include "Core/LogCategory.h"
#include "Core/DiagnosticMacros.h"
#include "Core/LoopExecutionTrace.h"
#include <thread>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <string>

namespace we::runtime::core {

// ============================================================================
// CommandQueue Implementation
// ============================================================================

bool CommandQueue::IsCoalesceable(ApplicationCommand::Type type) noexcept {
    return type == ApplicationCommand::Type::SwapchainRecreateRequest
        || type == ApplicationCommand::Type::GpuPresentRequest;
}

bool CommandQueue::IsFocusTransition(ApplicationCommand::Type type) noexcept {
    return type == ApplicationCommand::Type::WindowFocusLost
        || type == ApplicationCommand::Type::WindowFocusGained;
}

bool CommandQueue::IsMinimizeTransition(ApplicationCommand::Type type) noexcept {
    return type == ApplicationCommand::Type::WindowMinimized
        || type == ApplicationCommand::Type::WindowRestored;
}

void CommandQueue::Push(const ApplicationCommand& command) {
    const auto waitStart = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(m_Mutex);
    LoopExecutionTrace::MutexWait(
        "CommandQueue.Push",
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - waitStart).count());

    // Idempotent GPU/swapchain requests: keep a single pending instance.
    if (IsCoalesceable(command.type)) {
        for (const auto& pending : m_Queue) {
            if (pending.type == command.type) {
                return;
            }
        }
        m_Queue.push_back(command);
        return;
    }

    // Focus/minimize cascades are last-wins so alt-tab / system-chrome chatter
    // cannot thrash WindowState or throttling with stale intermediate states.
    if (IsFocusTransition(command.type) || IsMinimizeTransition(command.type)) {
        const bool focus = IsFocusTransition(command.type);
        m_Queue.erase(
            std::remove_if(m_Queue.begin(), m_Queue.end(),
                [focus](const ApplicationCommand& pending) {
                    return focus ? IsFocusTransition(pending.type)
                                 : IsMinimizeTransition(pending.type);
                }),
            m_Queue.end());
        m_Queue.push_back(command);
        return;
    }

    m_Queue.push_back(command);
}

std::vector<ApplicationCommand> CommandQueue::PopAll() {
    const auto waitStart = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(m_Mutex);
    LoopExecutionTrace::MutexWait(
        "CommandQueue.PopAll",
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - waitStart).count());
    std::vector<ApplicationCommand> commands;
    commands.reserve(m_Queue.size());
    for (auto& command : m_Queue) {
        commands.push_back(std::move(command));
    }
    m_Queue.clear();
    return commands;
}

bool CommandQueue::HasPending() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return !m_Queue.empty();
}

// ============================================================================
// WindowStateSubsystem Implementation
// ============================================================================

WindowStateSubsystem::WindowStateSubsystem() {
    // Initialize with default focused state
    m_IsFocused.store(true);
    m_IsMinimized.store(false);
    m_ShouldRender.store(true);
}

void WindowStateSubsystem::Initialize() {
    HE_INFO("[WindowStateSubsystem] Initialized");
}

void WindowStateSubsystem::Tick(float deltaTime) {
    (void)deltaTime;
    // Window state is updated via commands, no per-frame work needed
}

void WindowStateSubsystem::Shutdown() {
    HE_INFO("[WindowStateSubsystem] Shutdown");
}

void WindowStateSubsystem::ProcessCommand(const ApplicationCommand& command) {
    switch (command.type) {
        case ApplicationCommand::Type::WindowFocusLost:
            {
                std::lock_guard<std::mutex> lock(m_StateMutex);
                m_IsFocused.store(false);
                // Background rendering continues while unfocused (throttled by ThrottlingSubsystem).
                // Only window minimization should pause m_ShouldRender.
                HE_INFO("[WindowStateSubsystem] Window focus lost");
            }
            break;
            
        case ApplicationCommand::Type::WindowFocusGained:
            {
                std::lock_guard<std::mutex> lock(m_StateMutex);
                m_IsFocused.store(true);
                m_ShouldRender.store(true);
                HE_INFO("[WindowStateSubsystem] Window focus gained");
            }
            break;
            
        case ApplicationCommand::Type::WindowMinimized:
            {
                std::lock_guard<std::mutex> lock(m_StateMutex);
                m_IsMinimized.store(true);
                m_ShouldRender.store(false);
                HE_INFO("[WindowStateSubsystem] Window minimized");
            }
            break;
            
        case ApplicationCommand::Type::WindowRestored:
            {
                std::lock_guard<std::mutex> lock(m_StateMutex);
                m_IsMinimized.store(false);
                if (m_IsFocused.load()) {
                    m_ShouldRender.store(true);
                }
                HE_INFO("[WindowStateSubsystem] Window restored");
            }
            break;
            
        default:
            break;
    }
}

bool WindowStateSubsystem::IsWindowFocused() const {
    return m_IsFocused.load();
}

bool WindowStateSubsystem::IsWindowMinimized() const {
    return m_IsMinimized.load();
}

bool WindowStateSubsystem::ShouldRender() const {
    return m_ShouldRender.load();
}

// ============================================================================
// SwapchainSubsystem Implementation
// ============================================================================

SwapchainSubsystem::SwapchainSubsystem() {
    m_NeedsRecreation.store(false);
    m_IsRecreating.store(false);
}

void SwapchainSubsystem::Initialize() {
    HE_INFO("[SwapchainSubsystem] Initialized");
}

void SwapchainSubsystem::Tick(float deltaTime) {
    (void)deltaTime;

    // Single-flight with drain: concurrent ProcessCommand / nested Ensure calls
    // re-arm m_NeedsRecreation; collapse them into at most one extra pass.
    if (m_IsRecreating.exchange(true, std::memory_order_acq_rel)) {
        return;
    }

    RecreateSwapchainCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_CallbackMutex);
        callback = m_RecreateCallback;
    }

    while (m_NeedsRecreation.exchange(false, std::memory_order_acq_rel)) {
        if (callback) {
            const bool success = callback();
            if (success) {
                HE_INFO("[SwapchainSubsystem] Swapchain recreation completed successfully");
            } else {
                HE_ERROR("[SwapchainSubsystem] Swapchain recreation failed");
            }
        }
    }

    m_IsRecreating.store(false, std::memory_order_release);
}

void SwapchainSubsystem::Shutdown() {
    HE_INFO("[SwapchainSubsystem] Shutdown");
}

void SwapchainSubsystem::ProcessCommand(const ApplicationCommand& command) {
    switch (command.type) {
        case ApplicationCommand::Type::SwapchainRecreateRequest:
            {
                // Deduplicate: focus + resize + restore often enqueue the same request
                // several times in one pump; collapse to a single recreation pass.
                if (m_IsRecreating.load(std::memory_order_acquire)) {
                    // In-flight Ensure — re-arm so Tick runs one more pass after.
                    m_NeedsRecreation.store(true, std::memory_order_release);
                    return;
                }
                bool expected = false;
                if (!m_NeedsRecreation.compare_exchange_strong(
                        expected, true, std::memory_order_acq_rel)) {
                    // Already pending — ignore duplicate.
                    return;
                }
                HE_INFO("[SwapchainSubsystem] Swapchain recreation requested");
            }
            break;
            
        case ApplicationCommand::Type::GpuPresentRequest:
            {
                HE_INFO("[SwapchainSubsystem] GPU present requested");
            }
            break;
            
        default:
            break;
    }
}

bool SwapchainSubsystem::NeedsRecreation() const {
    return m_NeedsRecreation.load();
}

bool SwapchainSubsystem::IsRecreating() const {
    return m_IsRecreating.load(std::memory_order_acquire);
}

void SwapchainSubsystem::MarkRecreationComplete() {
    m_NeedsRecreation.store(false);
    m_IsRecreating.store(false);
}

void SwapchainSubsystem::SetRecreateCallback(RecreateSwapchainCallback callback) {
    std::lock_guard<std::mutex> lock(m_CallbackMutex);
    m_RecreateCallback = std::move(callback);
}

// ============================================================================
// ThrottlingSubsystem Implementation
// ============================================================================

ThrottlingSubsystem::ThrottlingSubsystem() {
    m_LastFrameTime = std::chrono::steady_clock::now();
}

void ThrottlingSubsystem::Initialize() {
    HE_INFO("[ThrottlingSubsystem] Initialized");
    HE_INFO(std::string("[ThrottlingSubsystem] Background FPS: ") + std::to_string(m_BackgroundFrameRate) +
            ", Minimized FPS: " + std::to_string(m_MinimizedFrameRate) +
            ", Focused FPS: " + std::to_string(m_FocusedFrameRate));
}

void ThrottlingSubsystem::Tick(float deltaTime) {
    (void)deltaTime;
    ThrottleFrame();
}

void ThrottlingSubsystem::Shutdown() {
    HE_INFO("[ThrottlingSubsystem] Shutdown");
}

void ThrottlingSubsystem::ProcessCommand(const ApplicationCommand& command) {
    switch (command.type) {
        case ApplicationCommand::Type::WindowFocusLost:
            m_CurrentTargetFrameRate = m_BackgroundFrameRate;
            HE_INFO(std::string("[ThrottlingSubsystem] Switched to background throttling (") +
                    std::to_string(m_BackgroundFrameRate) + " FPS)");
            break;
            
        case ApplicationCommand::Type::WindowFocusGained:
            m_CurrentTargetFrameRate = m_FocusedFrameRate;
            HE_INFO(std::string("[ThrottlingSubsystem] Switched to focused throttling (") +
                    std::to_string(m_FocusedFrameRate) + " FPS)");
            break;
            
        case ApplicationCommand::Type::WindowMinimized:
            m_CurrentTargetFrameRate = m_MinimizedFrameRate;
            HE_INFO(std::string("[ThrottlingSubsystem] Switched to minimized throttling (") +
                    std::to_string(m_MinimizedFrameRate) + " FPS)");
            break;
            
        case ApplicationCommand::Type::WindowRestored:
            // Will be updated by focus state
            break;
            
        default:
            break;
    }
}

void ThrottlingSubsystem::SetBackgroundFrameRate(float fps) {
    m_BackgroundFrameRate = fps;
}

void ThrottlingSubsystem::SetMinimizedFrameRate(float fps) {
    m_MinimizedFrameRate = fps;
}

void ThrottlingSubsystem::SetFocusedFrameRate(float fps) {
    m_FocusedFrameRate = fps;
}

void ThrottlingSubsystem::ThrottleFrame() {
    if (m_CurrentTargetFrameRate <= 0.0f) {
        return; // No throttling
    }
    
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - m_LastFrameTime);
    const auto targetDuration = std::chrono::microseconds(static_cast<int64_t>(1e6 / m_CurrentTargetFrameRate));
    
    if (elapsed < targetDuration) {
        const auto sleepDuration = targetDuration - elapsed;
        std::this_thread::sleep_for(sleepDuration);
    }
    
    m_LastFrameTime = std::chrono::steady_clock::now();
}

// ============================================================================
// ApplicationFramework Implementation
// ============================================================================

ApplicationFramework::ApplicationFramework() {
    m_Running.store(false);
    m_ShutdownRequested.store(false);
}

ApplicationFramework::~ApplicationFramework() {
    if (m_Running.load()) {
        HE_WARN("[ApplicationFramework] Destroying framework while still running");
    }
}

ISubsystem* ApplicationFramework::FindSubsystemByName(const char* name) const {
    if (!name) {
        return nullptr;
    }
    for (const auto& subsystem : m_Subsystems) {
        if (subsystem && subsystem->GetName() && std::strcmp(subsystem->GetName(), name) == 0) {
            return subsystem.get();
        }
    }
    return nullptr;
}

void ApplicationFramework::RegisterSubsystem(std::unique_ptr<ISubsystem> subsystem) {
    if (subsystem) {
        HE_INFO(std::string("[ApplicationFramework] Registering subsystem: ") + subsystem->GetName());
        m_Subsystems.push_back(std::move(subsystem));

        // Sort subsystems by priority (higher priority first)
        std::sort(m_Subsystems.begin(), m_Subsystems.end(),
            [](const auto& a, const auto& b) {
                return a->GetPriority() > b->GetPriority();
            });
    }
}

void ApplicationFramework::Initialize() {
    if (m_Initialized) {
        return;
    }
    HE_INFO("[ApplicationFramework] Initializing");
    InitializeApplication();
    InitializeSubsystems();
    m_Initialized = true;
    m_Running.store(true);
    m_ShutdownRequested.store(false);
}

bool ApplicationFramework::PollEvents() {
    return ProcessPlatformEvents();
}

bool ApplicationFramework::Tick(float deltaTime) {
    // LoopExecutionTrace scopes (WE_LOOP_TRACE=1) wrap pump → commands → subsystems.
    LoopExecutionTrace::Scoped tickScope("ApplicationFramework.Tick");
    if (!ShouldContinueRunning()) {
        return false;
    }

    BeginFrame();

    if (!ProcessPlatformEvents()) {
        LoopExecutionTrace::Event("ApplicationFramework.Tick", "ProcessPlatformEvents=false → RequestExit");
        RequestExit();
    }

    {
        LoopExecutionTrace::Scoped cmdScope("ApplicationFramework.ProcessCommands");
        ProcessCommands();
    }

    if (!ShouldContinueRunning()) {
        EndFrame();
        return false;
    }

    float clampedDt = deltaTime;
    if (clampedDt > 0.1f) {
        clampedDt = 0.1f;
    }

    TickApplication(clampedDt);
    {
        LoopExecutionTrace::Scoped subScope("ApplicationFramework.TickSubsystems");
        TickSubsystems(clampedDt);
    }
    EndFrame();

    return ShouldContinueRunning();
}

void ApplicationFramework::Shutdown() {
    if (!m_Initialized) {
        return;
    }
    ShutdownSubsystems();
    ShutdownApplication();
    m_Running.store(false);
    m_Initialized = false;
    HE_INFO("[ApplicationFramework] Shutdown complete");
}

void ApplicationFramework::RequestExit() {
    RequestShutdown();
}

void ApplicationFramework::Run() {
    HE_INFO("[ApplicationFramework] Starting application framework");

    Initialize();

    auto lastTime = std::chrono::steady_clock::now();
    while (ShouldContinueRunning()) {
        const auto now = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        if (deltaTime > 0.1f) {
            deltaTime = 0.1f;
        }

        if (!Tick(deltaTime)) {
            break;
        }
    }

    Shutdown();
    HE_INFO("[ApplicationFramework] Application framework stopped");
}

void ApplicationFramework::RequestShutdown() {
    HE_INFO("[ApplicationFramework] Shutdown requested");
    m_ShutdownRequested.store(true);
}

void ApplicationFramework::PushCommand(const ApplicationCommand& command) {
    m_CommandQueue.Push(command);
}

void ApplicationFramework::InitializeSubsystems() {
    HE_INFO(std::string("[ApplicationFramework] Initializing ") + std::to_string(m_Subsystems.size()) +
            " subsystems");
    
    for (auto& subsystem : m_Subsystems) {
        try {
            subsystem->Initialize();
        } catch (const std::exception& e) {
            HE_ERROR(std::string("[ApplicationFramework] Failed to initialize subsystem ") +
                     subsystem->GetName() + ": " + e.what());
        }
    }
}

void ApplicationFramework::TickSubsystems(float deltaTime) {
    for (auto& subsystem : m_Subsystems) {
        try {
            subsystem->Tick(deltaTime);
        } catch (const std::exception& e) {
            HE_ERROR(std::string("[ApplicationFramework] Failed to tick subsystem ") +
                     subsystem->GetName() + ": " + e.what());
        }
    }
}

void ApplicationFramework::ShutdownSubsystems() {
    HE_INFO("[ApplicationFramework] Shutting down subsystems");
    
    // Shutdown in reverse order (low priority first)
    for (auto it = m_Subsystems.rbegin(); it != m_Subsystems.rend(); ++it) {
        try {
            (*it)->Shutdown();
        } catch (const std::exception& e) {
            HE_ERROR(std::string("[ApplicationFramework] Failed to shutdown subsystem ") +
                     (*it)->GetName() + ": " + e.what());
        }
    }
}

void ApplicationFramework::ProcessCommands() {
    if (!m_CommandQueue.HasPending()) {
        return;
    }
    
    auto commands = m_CommandQueue.PopAll();
    
    for (const auto& command : commands) {
        // Process shutdown command immediately
        if (command.type == ApplicationCommand::Type::ShutdownRequest) {
            RequestShutdown();
            continue;
        }
        
        // Forward commands to all subsystems
        for (auto& subsystem : m_Subsystems) {
            try {
                subsystem->ProcessCommand(command);
            } catch (const std::exception& e) {
                HE_ERROR(std::string("[ApplicationFramework] Failed to process command in subsystem ") +
                         subsystem->GetName() + ": " + e.what());
            }
        }
    }
}

bool ApplicationFramework::ShouldContinueRunning() const {
    return m_Running.load() && !m_ShutdownRequested.load();
}

bool ApplicationFramework::IsRunning() const {
    return m_Running.load() && m_Initialized && !m_ShutdownRequested.load();
}

bool ApplicationFramework::ProcessPlatformEvents() {
    // Default: keep running. Derived classes poll the platform and return false on quit.
    return true;
}

void ApplicationFramework::BeginFrame() {
    // Default implementation - derived classes can override
}

void ApplicationFramework::EndFrame() {
    // Default implementation - derived classes can override
}

} // namespace we::runtime::core