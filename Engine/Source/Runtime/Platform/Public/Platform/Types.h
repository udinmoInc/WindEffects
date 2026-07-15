#pragma once

#include "Platform/Export.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace we::platform {

// ---------------------------------------------------------------------------
// Opaque handles (never expose OS types outside the Platform module)
// ---------------------------------------------------------------------------

enum class WindowId : uint32_t { Invalid = 0 };
enum class MonitorId : uint32_t { Invalid = 0 };
enum class ThreadId : uint64_t { Invalid = 0 };
enum class DynamicLibraryId : uint32_t { Invalid = 0 };
enum class DirectoryWatcherId : uint32_t { Invalid = 0 };
enum class TimerId : uint32_t { Invalid = 0 };
enum class CursorId : uint32_t { Invalid = 0 };

struct Int2 {
    int32_t x = 0;
    int32_t y = 0;
};

struct Uint2 {
    uint32_t x = 0;
    uint32_t y = 0;
};

struct Rect2i {
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;
};

struct Float2 {
    float x = 0.0f;
    float y = 0.0f;
};

// ---------------------------------------------------------------------------
// Application / window descriptors (designated-initializer friendly)
// ---------------------------------------------------------------------------

struct PlatformDesc {
    const char* appName = "WindEffects";
    const char* appVersion = "0.0.0";
    bool highDpiAware = true;
    bool enableRawInput = true;
    bool enableGamepad = true;
    bool enableTouch = true;
    // Enable only the services this process needs (tools / headless).
    // Include Capabilities.h for PlatformService; stored as uint32_t to keep Types.h thin.
    uint32_t services = 0xFFFFFFFFu; // PlatformService::All
    bool enableEventCoalescing = true;
    bool enableDiagnostics = true;
};

// Platform-agnostic borderless chrome hints (backends ignore unsupported fields).
struct WindowChromeDesc {
    bool roundedCorners = true;
    uint32_t borderColorRgb = 0x303030; // 0xRRGGBB
};

// Launch an external process without embedding OS process APIs in Core / Tools.
struct ProcessLaunchDesc {
    const char* executable = "";
    std::vector<std::string> arguments{};
    const char* workingDirectory = "";
    bool waitForExit = false;
    bool detach = true;
};

struct ProcessLaunchResult {
    bool launched = false;
    uint32_t processId = 0;
    int32_t exitCode = 0; // valid when waitForExit && launched
};

struct WindowDesc {
    const char* title = "WindEffects";
    int32_t width = 1280;
    int32_t height = 720;
    int32_t x = -1; // -1 = centered
    int32_t y = -1;
    bool resizable = true;
    bool maximized = false;
    bool minimized = false;
    bool borderless = false;
    bool fullscreen = false;
    bool visible = true;
    bool highDpi = true;
    bool acceptDropFiles = true;
    MonitorId monitor = MonitorId::Invalid;
};

enum class MessageBoxType : uint8_t {
    Info,
    Warning,
    Error,
    Question
};

enum class MessageBoxResult : uint8_t {
    Ok,
    Cancel,
    Yes,
    No
};

struct MessageBoxDesc {
    const char* title = "WindEffects";
    const char* message = "";
    MessageBoxType type = MessageBoxType::Info;
    bool yesNo = false;
    bool okCancel = false;
};

enum class FileDialogMode : uint8_t {
    OpenFile,
    OpenFiles,
    SaveFile,
    OpenFolder
};

struct FileFilter {
    const char* name = "All Files";
    const char* pattern = "*.*"; // e.g. "*.png;*.jpg"
};

struct FileDialogDesc {
    FileDialogMode mode = FileDialogMode::OpenFile;
    const char* title = "Select File";
    const char* defaultPath = "";
    const char* defaultName = "";
    std::vector<FileFilter> filters{};
};

struct DisplayMode {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t refreshRateHz = 0;
    uint32_t bitsPerPixel = 32;
};

struct MonitorInfo {
    MonitorId id = MonitorId::Invalid;
    std::string name;
    Rect2i bounds{};
    Rect2i workArea{};
    float dpiScale = 1.0f;
    uint32_t dpi = 96;
    bool primary = false;
    std::vector<DisplayMode> modes{};
};

struct MemoryInfo {
    uint64_t totalPhysicalBytes = 0;
    uint64_t availablePhysicalBytes = 0;
    uint64_t totalVirtualBytes = 0;
    uint64_t availableVirtualBytes = 0;
    uint64_t processUsedBytes = 0;
};

struct CpuInfo {
    std::string brand;
    uint32_t physicalCores = 0;
    uint32_t logicalCores = 0;
    uint32_t pageSize = 0;
    bool hyperThreading = false;
};

struct GpuAdapterInfo {
    uint32_t index = 0;
    std::string name;
    uint64_t dedicatedVideoMemory = 0;
    uint64_t dedicatedSystemMemory = 0;
    uint64_t sharedSystemMemory = 0;
    uint32_t vendorId = 0;
    uint32_t deviceId = 0;
    bool isSoftware = false;
};

enum class ThreadPriority : uint8_t {
    Lowest,
    BelowNormal,
    Normal,
    AboveNormal,
    Highest,
    TimeCritical
};

enum class SystemCursor : uint8_t {
    Arrow,
    IBeam,
    Wait,
    Crosshair,
    Hand,
    SizeNS,
    SizeWE,
    SizeNWSE,
    SizeNESW,
    SizeAll,
    No,
    Hidden
};

enum class DirectoryWatchAction : uint8_t {
    Added,
    Removed,
    Modified,
    RenamedOld,
    RenamedNew
};

// Borderless-window non-client hit testing (maps to OS hit codes in backends).
enum class WindowHitTestResult : uint8_t {
    Client,
    Draggable,
    ResizeTopLeft,
    ResizeTop,
    ResizeTopRight,
    ResizeRight,
    ResizeBottomRight,
    ResizeBottom,
    ResizeBottomLeft,
    ResizeLeft
};

using WindowHitTestFn = WindowHitTestResult (*)(WindowId window, Int2 point, void* userData);

// Opaque sync primitives — concrete storage lives in the platform backend.
struct SyncMutex;
struct SyncEvent;

} // namespace we::platform
