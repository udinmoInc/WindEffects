// ==============================================================================
// WindEffects — Core — ApplicationFramework
// Decoupled application framework for engine applications (Editor and Runtime)
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Core/IEngineLoop.h"
#include "Core/Export.h"

#include <memory>
#include <vector>
#include <deque>
#include <mutex>
#include <atomic>
#include <functional>
#include <variant>
#include <chrono>
#include <string>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif

namespace we::runtime::core {

// Forward declarations
class ISubsystem;

/**
 * Command types for the deferred command queue
 * These commands are pushed from event handlers and executed during the main tick
 */
struct ApplicationCommand {
    enum class Type {
        WindowFocusLost,
        WindowFocusGained,
        WindowMinimized,
        WindowRestored,
        SwapchainRecreateRequest,
        GpuPresentRequest,
        ShutdownRequest
    };

    Type type;
    std::variant<bool, uint32_t, uint64_t> data; // Optional parameter data

    ApplicationCommand(Type cmdType) : type(cmdType) {}
    ApplicationCommand(Type cmdType, bool boolData) : type(cmdType), data(boolData) {}
    ApplicationCommand(Type cmdType, uint32_t uintData) : type(cmdType), data(uintData) {}
};

/**
 * Thread-safe command queue for deferred event processing
 * Events can be pushed from any thread and will be processed during the main tick
 */
class CORE_API CommandQueue {
public:
    CommandQueue() = default;
    ~CommandQueue() = default;

    // Push a command to the queue (thread-safe).
    // Coalesces SwapchainRecreateRequest / GpuPresentRequest so focus+resize
    // cascades collapse to a single pending command of each type.
    // WindowFocusLost/Gained and Minimized/Restored keep only the latest (last-wins).
    void Push(const ApplicationCommand& command);
    
    // Pop all pending commands (clears the queue)
    std::vector<ApplicationCommand> PopAll();
    
    // Check if there are pending commands
    bool HasPending() const;

private:
    [[nodiscard]] static bool IsCoalesceable(ApplicationCommand::Type type) noexcept;
    [[nodiscard]] static bool IsFocusTransition(ApplicationCommand::Type type) noexcept;
    [[nodiscard]] static bool IsMinimizeTransition(ApplicationCommand::Type type) noexcept;

    mutable std::mutex m_Mutex;
    std::deque<ApplicationCommand> m_Queue;
};

/**
 * Base interface for all engine subsystems
 * Subsystems provide modular functionality and can be registered with the application framework
 */
class ISubsystem {
public:
    virtual ~ISubsystem() = default;

    // Called when the subsystem is initialized
    virtual void Initialize() = 0;
    
    // Called every frame with delta time
    virtual void Tick(float deltaTime) = 0;
    
    // Called when the application is shutting down
    virtual void Shutdown() = 0;
    
    // Optional: Process application commands
    virtual void ProcessCommand(const ApplicationCommand& command) {}
    
    // Get subsystem priority (higher values tick earlier)
    virtual int GetPriority() const { return 0; }
    
    // Get subsystem name for debugging
    virtual const char* GetName() const { return "UnnamedSubsystem"; }
};

/**
 * Window state management subsystem
 * Handles window focus, minimize events without blocking the main thread
 */
class CORE_API WindowStateSubsystem : public ISubsystem {
public:
    WindowStateSubsystem();
    ~WindowStateSubsystem() override = default;

    void Initialize() override;
    void Tick(float deltaTime) override;
    void Shutdown() override;
    void ProcessCommand(const ApplicationCommand& command) override;
    
    const char* GetName() const override { return "WindowStateSubsystem"; }
    int GetPriority() const override { return 100; } // High priority - process first

    // Query current window state (thread-safe)
    bool IsWindowFocused() const;
    bool IsWindowMinimized() const;
    bool ShouldRender() const;

private:
    mutable std::mutex m_StateMutex;
    std::atomic<bool> m_IsFocused{true};
    std::atomic<bool> m_IsMinimized{false};
    std::atomic<bool> m_ShouldRender{true};
};

/**
 * Swapchain management subsystem
 * Handles swapchain recreation requests asynchronously
 */
class CORE_API SwapchainSubsystem : public ISubsystem {
public:
    SwapchainSubsystem();
    ~SwapchainSubsystem() override = default;

    void Initialize() override;
    void Tick(float deltaTime) override;
    void Shutdown() override;
    void ProcessCommand(const ApplicationCommand& command) override;
    
    const char* GetName() const override { return "SwapchainSubsystem"; }
    int GetPriority() const override { return 50; } // Medium priority

    // Check if swapchain recreation is pending
    bool NeedsRecreation() const;
    [[nodiscard]] bool IsRecreating() const;
    void MarkRecreationComplete();
    
    // Set renderer callback for swapchain operations
    using RecreateSwapchainCallback = std::function<bool()>;
    void SetRecreateCallback(RecreateSwapchainCallback callback);

private:
    mutable std::mutex m_CallbackMutex;
    RecreateSwapchainCallback m_RecreateCallback;
    std::atomic<bool> m_NeedsRecreation{false};
    std::atomic<bool> m_IsRecreating{false};
};

/**
 * Background throttling subsystem
 * Manages CPU/GPU throttling when window is unfocused or minimized
 * Uses sleep-based throttling without blocking the main thread
 */
class CORE_API ThrottlingSubsystem : public ISubsystem {
public:
    ThrottlingSubsystem();
    ~ThrottlingSubsystem() override = default;

    void Initialize() override;
    void Tick(float deltaTime) override;
    void Shutdown() override;
    void ProcessCommand(const ApplicationCommand& command) override;
    
    const char* GetName() const override { return "ThrottlingSubsystem"; }
    int GetPriority() const override { return 0; } // Low priority - process last

    // Configure throttling behavior
    void SetBackgroundFrameRate(float fps);
    void SetMinimizedFrameRate(float fps);
    void SetFocusedFrameRate(float fps);

private:
    std::chrono::steady_clock::time_point m_LastFrameTime;
    float m_BackgroundFrameRate = 30.0f;
    float m_MinimizedFrameRate = 5.0f;
    /// 0 = uncapped when focused (no sleep). VSync/present pacing is separate.
    float m_FocusedFrameRate = 0.0f;
    float m_CurrentTargetFrameRate = 0.0f;
    
    void ThrottleFrame();
};

/**
 * Base application framework
 * Provides a reusable application architecture for both Editor and Runtime apps
 * Implements a decoupled subsystem system with deferred command processing
 */
class CORE_API ApplicationFramework : public IEngineLoop {
public:
    ApplicationFramework();
    ~ApplicationFramework() override;

    // Register a subsystem with the application
    void RegisterSubsystem(std::unique_ptr<ISubsystem> subsystem);

    /// Find a registered subsystem by dynamic type (requires RTTI).
    template <typename T>
    T* FindSubsystem() const {
        for (const auto& subsystem : m_Subsystems) {
            if (auto* typed = dynamic_cast<T*>(subsystem.get())) {
                return typed;
            }
        }
        return nullptr;
    }

    /// Find a registered subsystem by GetName().
    ISubsystem* FindSubsystemByName(const char* name) const;

    // IEngineLoop
    void Initialize() override;
    bool PollEvents() override;
    bool Tick(float deltaTime) override;
    void Shutdown() override;
    void RequestExit() override;
    void Run() override;
    bool IsRunning() const override;

    // Request application shutdown (alias for RequestExit)
    void RequestShutdown();

    // Push a command to the deferred queue (thread-safe)
    void PushCommand(const ApplicationCommand& command);

    // Get the command queue for external use
    CommandQueue& GetCommandQueue() { return m_CommandQueue; }

protected:
    // Virtual methods for derived classes to implement
    virtual void InitializeApplication() = 0;
    virtual void TickApplication(float deltaTime) = 0;
    virtual void ShutdownApplication() = 0;

    // Optional: Override to customize main loop behavior
    virtual bool ShouldContinueRunning() const;
    /// Pump platform messages; return false to stop the loop.
    virtual bool ProcessPlatformEvents();
    virtual void BeginFrame();
    virtual void EndFrame();

    void ProcessCommands();
    void TickSubsystems(float deltaTime);

private:
    void InitializeSubsystems();
    void ShutdownSubsystems();

    std::vector<std::unique_ptr<ISubsystem>> m_Subsystems;
    CommandQueue m_CommandQueue;
    std::atomic<bool> m_Running{false};
    std::atomic<bool> m_ShutdownRequested{false};
    bool m_Initialized = false;
};

} // namespace we::runtime::core

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
