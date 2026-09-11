# WindEffects Application Framework Architecture

## Overview

The WindEffects Application Framework provides a robust, decoupled subsystem architecture similar to Unreal Engine's modular design. This architecture eliminates tight coupling between window state management, rendering, and the main application loop by introducing a deferred command queue and independent subsystems.

## Architecture Components

### 1. Core Framework (`ApplicationFramework`)

The base application framework provides a reusable foundation for both Editor and Runtime applications:

- **Decoupled Main Loop**: Centralized tick system with deferred command processing
- **Subsystem Management**: Modular subsystem registration with priority-based execution
- **Thread-Safe Command Queue**: Asynchronous event handling without blocking the main thread
- **Lifecycle Management**: Proper initialization, ticking, and shutdown of all subsystems

### 2. Command Queue (`CommandQueue`)

A thread-safe command queue for deferred event processing:

- **Thread-Safe Operations**: Commands can be pushed from any thread
- **Deferred Processing**: Events are processed during the main tick, avoiding blocking
- **Command Types**: Window state changes, swapchain requests, shutdown, etc.

### 3. Subsystem Interface (`ISubsystem`)

Base interface for all engine subsystems:

```cpp
class ISubsystem {
public:
    virtual void Initialize() = 0;
    virtual void Tick(float deltaTime) = 0;
    virtual void Shutdown() = 0;
    virtual void ProcessCommand(const ApplicationCommand& command) {}
    virtual int GetPriority() const { return 0; }
    virtual const char* GetName() const { return "UnnamedSubsystem"; }
};
```

### 4. Window State Subsystem (`WindowStateSubsystem`)

Manages window focus and minimize state without blocking:

- **Thread-Safe State**: Atomic operations for window state queries
- **Deferred Processing**: Window events pushed to command queue, processed during tick
- **Render Control**: Determines when rendering should occur based on window state
- **High Priority**: Processes before other subsystems (priority: 100)

### 5. Swapchain Subsystem (`SwapchainSubsystem`)

Handles Vulkan swapchain recreation asynchronously:

- **Deferred Recreation**: Swapchain requests pushed to command queue
- **Callback Integration**: Pluggable recreation callback for Editor integration
- **State Tracking**: Prevents concurrent recreation attempts
- **Medium Priority**: Processes after window state (priority: 50)

### 6. Throttling Subsystem (`ThrottlingSubsystem`)

Manages CPU/GPU throttling based on window state:

- **Adaptive Frame Rates**: Different throttling for focused, background, and minimized states
- **Non-Blocking**: Uses sleep-based throttling without blocking the main thread
- **Configurable**: Adjustable frame rates for different states
- **Low Priority**: Processes last (priority: 0)

## Migration from Old Architecture

### Before (Tightly Coupled)

```cpp
// Old EditorMainLoop.cpp - Direct blocking operations
while (m_Running) {
    // Direct event handling mixed with rendering
    for (const auto& event : frameEvents) {
        if (auto* focus = std::get_if<we::platform::WindowFocusEvent>(&event)) {
            if (!focus->focused) {
                m_AppFocused = false; // Hardcoded state flag
                // Direct UI cleanup
            }
        }
    }
    
    // Inline swapchain checks
    const bool allowGpu = !windowMinimized && windowFocused;
    if (!allowGpu) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // Blocking sleep
    } else {
        if (m_ForceSwapchainRecreate) { // Hardcoded state flag
            EnsureVisibleSwapchain();
        }
    }
}
```

### After (Decoupled Architecture)

```cpp
// New EditorMainLoop.cpp - Framework-based
void Editor::MainLoop() {
    m_ApplicationFramework = std::make_unique<EditorApplicationFramework>(this);
    m_ApplicationFramework->Run();
    m_ApplicationFramework.reset();
}

// Event handling in EditorApplicationFramework
void EditorApplicationFramework::ProcessPlatformEvents() {
    const auto frameEvents = platform.GetFrameEvents();
    
    for (const auto& event : frameEvents) {
        if (auto* focus = std::get_if<we::platform::WindowFocusEvent>(&event)) {
            if (focus->focused) {
                PushCommand(ApplicationCommand::Type::WindowFocusGained);
            } else {
                PushCommand(ApplicationCommand::Type::WindowFocusLost);
            }
        }
    }
}
```

## Key Benefits

### 1. Elimination of Blocking Operations

- **Before**: Thread sleep blocks entire render thread when window unfocused
- **After**: Throttling subsystem manages frame timing without blocking

### 2. Decoupled Window State Management

- **Before**: Window state flags hardcoded in Editor class
- **After**: Window state managed by dedicated subsystem with thread-safe queries

### 3. Asynchronous Swapchain Management

- **Before**: Inline swapchain checks in main loop
- **After**: Swapchain subsystem handles recreation requests asynchronously

### 4. Reusable Architecture

- **Before**: Editor-specific main loop tightly coupled to implementation
- **After**: Framework reusable for both Editor and standalone runtime applications

### 5. Improved Maintainability

- **Before**: Window events, rendering, and state management mixed in single loop
- **After**: Clear separation of concerns with dedicated subsystems

## Integration Guide

### For Editor Applications

1. **Create EditorApplicationFramework instance**:
```cpp
m_ApplicationFramework = std::make_unique<EditorApplicationFramework>(this);
```

2. **Implement framework integration methods**:
```cpp
void TickEditor(float deltaTime);
void ProcessEditorInputEvents();
void RenderEditorFrame();
```

3. **Access subsystems for queries**:
```cpp
auto* windowState = m_ApplicationFramework->GetWindowStateSubsystem();
bool shouldRender = windowState->ShouldRender();
```

### For Runtime Applications

1. **Extend ApplicationFramework**:
```cpp
class RuntimeApplicationFramework : public ApplicationFramework {
protected:
    void InitializeApplication() override;
    void TickApplication(float deltaTime) override;
    void ShutdownApplication() override;
};
```

2. **Register custom subsystems**:
```cpp
RegisterSubsystem(std::make_unique<CustomSubsystem>());
```

3. **Run the framework**:
```cpp
RuntimeApplicationFramework framework;
framework.Run();
```

## Performance Characteristics

### Frame Flow

1. **BeginFrame**: Initialize frame counters and diagnostics
2. **ProcessPlatformEvents**: Poll events and push commands to queue
3. **ProcessCommands**: Execute deferred commands in subsystems
4. **TickApplication**: Application-specific logic
5. **TickSubsystems**: Subsystem processing in priority order
6. **EndFrame**: Finalize frame and collect statistics

### Thread Safety

- **Command Queue**: Mutex-protected for thread-safe operations
- **Window State**: Atomic operations for lock-free queries
- **Swapchain State**: Atomic flags to prevent concurrent operations

### Memory Usage

- **Minimal Overhead**: Subsystem pointers stored in vector
- **Command Queue**: Efficient queue with batch processing
- **No Per-Frame Allocations**: Reuse of existing structures

## Troubleshooting

### Common Issues

1. **Window State Not Updating**
   - Ensure commands are being pushed for window events
   - Check that WindowStateSubsystem is registered with correct priority

2. **Swapchain Not Recreating**
   - Verify SwapchainSubsystem recreation callback is set
   - Check that commands are being pushed for resize events

3. **Throttling Not Working**
   - Ensure ThrottlingSubsystem is registered
   - Verify frame rate configuration is appropriate

### Debugging

Enable detailed logging by setting appropriate log levels:

```cpp
HE_INFO("[Subsystem] Window state changed");
HE_INFO("[Framework] Processing commands");
```

## Future Enhancements

### Planned Features

1. **Additional Subsystems**: Audio, networking, file I/O
2. **Plugin System**: Dynamic subsystem loading
3. **Profiling Integration**: Enhanced subsystem performance metrics
4. **Hot Reloading**: Subsystem reload during runtime

### Extension Points

The framework is designed for extensibility:

- **Custom Subsystems**: Implement ISubsystem for new functionality
- **Custom Commands**: Extend ApplicationCommand::Type for new events
- **Framework Overrides**: Virtual methods for specialized behavior

## Conclusion

The WindEffects Application Framework provides a robust foundation for engine applications with:

- **Decoupled Architecture**: Clear separation of concerns
- **Thread Safety**: Safe concurrent operations
- **Reusability**: Shared framework for Editor and Runtime
- **Maintainability**: Modular, testable components
- **Performance**: Efficient command processing and subsystem management

This architecture eliminates the tight coupling issues of the previous implementation while providing a solid foundation for future development.