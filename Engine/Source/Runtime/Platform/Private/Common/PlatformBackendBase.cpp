#include "Common/PlatformBackendBase.h"
#include "Common/SyncPrimitives.h"

#include "Platform/Platform.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"

#include <cstdio>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

namespace we::platform {
namespace {

void PlatformErrorDialog(const char* title, const char* message, bool fatal, void* /*userData*/) {
    if (!Platform::IsAvailable()) {
        return;
    }
    MessageBoxDesc desc{};
    desc.title = title ? title : "WindEffects";
    desc.message = message ? message : "";
    desc.type = MessageBoxType::Error;
    (void)Platform::Get().ShowMessageBox(desc);
    (void)fatal;
}

} // namespace

bool PlatformBackendBase::Initialize(const PlatformDesc& desc) {
    if (m_Initialized) {
        return true;
    }

    const auto initStart = std::chrono::steady_clock::now();
    m_Desc = desc;
    m_EnabledServices = static_cast<PlatformService>(desc.services);
    m_StartTime = initStart;
    m_WantsQuit = false;
    m_Events.Clear();
    m_Events.SetCoalescingEnabled(desc.enableEventCoalescing);
    m_Events.ResetCoalescedCount();
    ClearLastError();

    m_MainThreadId = GetCurrentThreadId();
    Detail_RegisterMainThread(static_cast<uint64_t>(m_MainThreadId));

    // Default portable capabilities; host backends refine after Initialize.
    m_Capabilities = {};
    m_Capabilities.threading = HasService(m_EnabledServices, PlatformService::Threading);
    m_Capabilities.dynamicLibraries = true;
    m_Capabilities.highResolutionTimer = HasService(m_EnabledServices, PlatformService::Timers);

    if (desc.enableDiagnostics) {
        m_Diagnostics.Reset();
    }

    m_Initialized = true;

    // Prefer Platform message boxes from Core Logger when available (no Core→Platform link).
    we::Logger::SetErrorDialogHandler(&PlatformErrorDialog, nullptr);

    const auto initEnd = std::chrono::steady_clock::now();
    m_Diagnostics.initializeTimeMs =
        std::chrono::duration<double, std::milli>(initEnd - initStart).count();
    m_Diagnostics.timerFrequencyHz = GetHighResolutionFrequency();
    if (m_Diagnostics.timerFrequencyHz > 0) {
        m_Diagnostics.timerPrecisionNs = 1.0e9 / static_cast<double>(m_Diagnostics.timerFrequencyHz);
    }

    WE_LOG_INFO(we::LogCategory::Startup, std::string("Platform backend initialized: ") + GetName());
    return true;
}

void PlatformBackendBase::Shutdown() {
    if (!m_Initialized) {
        return;
    }

    const auto shutdownStart = std::chrono::steady_clock::now();
    we::Logger::SetErrorDialogHandler(nullptr, nullptr);

    {
        std::scoped_lock lock(m_ThreadMutex);
        for (auto& [id, record] : m_Threads) {
            if (record.thread.joinable()) {
                record.thread.detach();
            }
        }
        m_Threads.clear();
    }
    for (auto& [id, handle] : m_Libraries) {
        (void)id;
        (void)handle;
    }
    m_Libraries.clear();
    m_Handlers.clear();
    m_Events.Clear();
    Detail_ClearMainThread();
    m_MainThreadId = ThreadId::Invalid;
    m_Initialized = false;

    const auto shutdownEnd = std::chrono::steady_clock::now();
    m_Diagnostics.shutdownTimeMs =
        std::chrono::duration<double, std::milli>(shutdownEnd - shutdownStart).count();
    WE_LOG_INFO(we::LogCategory::Startup, "Platform backend shut down.");
}

void PlatformBackendBase::ShutdownService(PlatformService service) {
    m_EnabledServices = static_cast<PlatformService>(
        static_cast<uint32_t>(m_EnabledServices) & ~static_cast<uint32_t>(service));
}

bool PlatformBackendBase::IsServiceEnabled(PlatformService service) const {
    return HasService(m_EnabledServices, service);
}

void PlatformBackendBase::SetError(PlatformError error) {
    m_LastError = std::move(error);
}

PlatformError PlatformBackendBase::MakeError(
    PlatformErrorCode code,
    std::string_view message,
    const char* api,
    int32_t osCode)
{
    auto err = PlatformError::Make(code, message, api, osCode);
    SetError(err);
    return err;
}

bool PlatformBackendBase::RequireService(PlatformService service, const char* api) {
    if (IsServiceEnabled(service)) {
        return true;
    }
    MakeError(PlatformErrorCode::ServiceDisabled, "Platform service is disabled for this process.", api);
    return false;
}

void PlatformBackendBase::BeginTimedOp() {
    m_OpStart = std::chrono::steady_clock::now();
}

double PlatformBackendBase::EndTimedOpMs() const {
    return std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - m_OpStart).count();
}

bool PlatformBackendBase::PollEvents() {
    AssertMainThread("PollEvents");
    BeginTimedOp();
    const uint64_t coalescedBefore = m_Events.CoalescedCount();
    m_Events.FlushToFrame();
    DispatchFrameEvents();
    m_Diagnostics.lastPollEventsMs = EndTimedOpMs();
    m_Diagnostics.totalPollEventsMs += m_Diagnostics.lastPollEventsMs;
    m_Diagnostics.eventsDispatched += static_cast<uint64_t>(m_Events.FrameEvents().size());
    m_Diagnostics.eventsCoalesced += (m_Events.CoalescedCount() - coalescedBefore);
    return !m_WantsQuit;
}

void PlatformBackendBase::WaitEvents(double timeoutSeconds) {
    AssertMainThread("WaitEvents");
    if (timeoutSeconds < 0.0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } else {
        const auto ms = static_cast<int64_t>(timeoutSeconds * 1000.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
    PollEvents();
}

void PlatformBackendBase::PostQuit() {
    m_WantsQuit = true;
    m_Events.Push(QuitEvent{});
}

void PlatformBackendBase::PushEvent(PlatformEvent event) {
    if (std::holds_alternative<QuitEvent>(event)) {
        m_WantsQuit = true;
    }
    m_Events.Push(std::move(event));
}

void PlatformBackendBase::AddEventHandler(EventHandler handler) {
    if (handler) {
        m_Handlers.push_back(std::move(handler));
    }
}

std::span<const PlatformEvent> PlatformBackendBase::GetFrameEvents() const {
    return m_Events.FrameEvents();
}

void PlatformBackendBase::DispatchFrameEvents() {
    for (const auto& event : m_Events.FrameEvents()) {
        if (std::holds_alternative<QuitEvent>(event)) {
            m_WantsQuit = true;
        }
        for (const auto& handler : m_Handlers) {
            handler(event);
        }
    }
}

WindowId PlatformBackendBase::AllocateWindowId() {
    return static_cast<WindowId>(m_NextWindowId++);
}

Result<WindowId> PlatformBackendBase::CreateWindow(const WindowDesc&) {
    AssertMainThread("CreateWindow");
    if (!RequireService(PlatformService::Windowing, "CreateWindow")) {
        return MakeError(PlatformErrorCode::ServiceDisabled, "Windowing service disabled.", "CreateWindow");
    }
    return MakeError(PlatformErrorCode::NotSupported, "CreateWindow not implemented on this platform backend.", "CreateWindow");
}

Result<void> PlatformBackendBase::DestroyWindow(WindowId) {
    AssertMainThread("DestroyWindow");
    return Result<void>::Success();
}

bool PlatformBackendBase::IsWindowValid(WindowId) const { return false; }
NativeWindowHandle PlatformBackendBase::GetNativeWindowHandle(WindowId) const { return {}; }

void PlatformBackendBase::SetWindowTitle(WindowId, std::string_view) {}
void PlatformBackendBase::SetWindowSize(WindowId, int32_t, int32_t) {}
void PlatformBackendBase::SetWindowPosition(WindowId, int32_t, int32_t) {}
void PlatformBackendBase::ShowWindow(WindowId) {}
void PlatformBackendBase::HideWindow(WindowId) {}
void PlatformBackendBase::FocusWindow(WindowId) {}
void PlatformBackendBase::MaximizeWindow(WindowId) {}
void PlatformBackendBase::MinimizeWindow(WindowId) {}
void PlatformBackendBase::RestoreWindow(WindowId) {}
void PlatformBackendBase::SetWindowBorderless(WindowId, bool) {}
void PlatformBackendBase::SetWindowFullscreen(WindowId, bool, MonitorId) {}

Result<void> PlatformBackendBase::ApplyWindowChrome(WindowId, const WindowChromeDesc&) {
    return MakeError(PlatformErrorCode::NotSupported, "ApplyWindowChrome not supported.", "ApplyWindowChrome");
}

Result<void> PlatformBackendBase::SetWindowIcon(WindowId, int32_t, const char*) {
    return MakeError(PlatformErrorCode::NotSupported, "SetWindowIcon not supported.", "SetWindowIcon");
}

Uint2 PlatformBackendBase::GetWindowSize(WindowId) const { return {}; }
Uint2 PlatformBackendBase::GetWindowPixelSize(WindowId) const { return {}; }
Int2 PlatformBackendBase::GetWindowPosition(WindowId) const { return {}; }
float PlatformBackendBase::GetWindowDpiScale(WindowId) const { return 1.0f; }
bool PlatformBackendBase::IsWindowFocused(WindowId) const { return false; }
bool PlatformBackendBase::IsWindowMaximized(WindowId) const { return false; }
bool PlatformBackendBase::IsWindowMinimized(WindowId) const { return false; }
std::vector<WindowId> PlatformBackendBase::GetWindows() const { return {}; }
void PlatformBackendBase::SetWindowHitTest(WindowId, WindowHitTestFn, void*) {}

bool PlatformBackendBase::IsKeyDown(KeyCode) const { return false; }
bool PlatformBackendBase::IsMouseButtonDown(MouseButton) const { return false; }
Int2 PlatformBackendBase::GetMousePosition() const { return {}; }
Int2 PlatformBackendBase::GetMousePosition(WindowId) const { return {}; }
void PlatformBackendBase::SetMousePosition(int32_t, int32_t) {}
void PlatformBackendBase::SetMousePosition(WindowId, int32_t, int32_t) {}
void PlatformBackendBase::SetRelativeMouseMode(WindowId, bool) {}
bool PlatformBackendBase::IsRelativeMouseMode(WindowId) const { return false; }
void PlatformBackendBase::SetCursorVisible(bool) {}
void PlatformBackendBase::SetSystemCursor(SystemCursor) {}
KeyModifier PlatformBackendBase::GetKeyModifiers() const { return KeyModifier::None; }

uint32_t PlatformBackendBase::GetGamepadCount() const { return 0; }
bool PlatformBackendBase::IsGamepadConnected(uint32_t) const { return false; }
bool PlatformBackendBase::IsGamepadButtonDown(uint32_t, GamepadButton) const { return false; }
float PlatformBackendBase::GetGamepadAxis(uint32_t, GamepadAxis) const { return 0.0f; }
void PlatformBackendBase::SetGamepadVibration(uint32_t, float, float) {}

Result<void> PlatformBackendBase::SetClipboardText(std::string_view) {
    return MakeError(PlatformErrorCode::NotSupported, "Clipboard not supported.", "SetClipboardText");
}

std::string PlatformBackendBase::GetClipboardText() const { return {}; }

MessageBoxResult PlatformBackendBase::ShowMessageBox(const MessageBoxDesc&) {
    return MessageBoxResult::Ok;
}

std::vector<std::string> PlatformBackendBase::ShowFileDialog(const FileDialogDesc&) {
    return {};
}

std::vector<MonitorInfo> PlatformBackendBase::GetMonitors() const { return {}; }
MonitorId PlatformBackendBase::GetPrimaryMonitor() const { return MonitorId::Invalid; }
MonitorInfo PlatformBackendBase::GetMonitorInfo(MonitorId) const { return {}; }

double PlatformBackendBase::GetTimeSeconds() const {
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(clock::now() - m_StartTime).count();
}

uint64_t PlatformBackendBase::GetTimeNanoseconds() const {
    using clock = std::chrono::steady_clock;
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now() - m_StartTime).count());
}

uint64_t PlatformBackendBase::GetHighResolutionCounter() const {
    return static_cast<uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
}

uint64_t PlatformBackendBase::GetHighResolutionFrequency() const {
    using period = std::chrono::steady_clock::period;
    return static_cast<uint64_t>(period::den) / static_cast<uint64_t>(period::num);
}

Result<TimerId> PlatformBackendBase::CreateTimer(double, bool) {
    return MakeError(PlatformErrorCode::NotSupported, "Timers not implemented.", "CreateTimer");
}

Result<void> PlatformBackendBase::DestroyTimer(TimerId) {
    return Result<void>::Success();
}

Result<ThreadId> PlatformBackendBase::CreateThread(ThreadFn fn, void* userData, std::string_view name) {
    if (!RequireService(PlatformService::Threading, "CreateThread")) {
        return m_LastError;
    }
    if (!fn) {
        return MakeError(PlatformErrorCode::InvalidArgument, "Thread function is null.", "CreateThread");
    }
    BeginTimedOp();
    std::scoped_lock lock(m_ThreadMutex);
    auto thread = std::thread(fn, userData);
    const uint64_t id = static_cast<uint64_t>(std::hash<std::thread::id>{}(thread.get_id()));
    m_Threads.emplace(id, ThreadRecord{std::move(thread), std::string(name)});
    m_Diagnostics.lastThreadCreateMs = EndTimedOpMs();
    ++m_Diagnostics.threadsCreated;
    ClearLastError();
    return static_cast<ThreadId>(id);
}

Result<void> PlatformBackendBase::JoinThread(ThreadId id) {
    std::scoped_lock lock(m_ThreadMutex);
    if (auto it = m_Threads.find(static_cast<uint64_t>(id)); it != m_Threads.end()) {
        if (it->second.thread.joinable()) {
            it->second.thread.join();
        }
        m_Threads.erase(it);
        ClearLastError();
        return Result<void>::Success();
    }
    return MakeError(PlatformErrorCode::InvalidHandle, "Unknown thread id.", "JoinThread");
}

Result<void> PlatformBackendBase::DetachThread(ThreadId id) {
    std::scoped_lock lock(m_ThreadMutex);
    if (auto it = m_Threads.find(static_cast<uint64_t>(id)); it != m_Threads.end()) {
        if (it->second.thread.joinable()) {
            it->second.thread.detach();
        }
        m_Threads.erase(it);
        ClearLastError();
        return Result<void>::Success();
    }
    return MakeError(PlatformErrorCode::InvalidHandle, "Unknown thread id.", "DetachThread");
}

void PlatformBackendBase::SetThreadName(std::string_view) {}
void PlatformBackendBase::SetThreadPriority(ThreadId, ThreadPriority) {}
void PlatformBackendBase::SetThreadAffinity(ThreadId, uint64_t) {}

ThreadId PlatformBackendBase::GetCurrentThreadId() const {
    return static_cast<ThreadId>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
}

ThreadId PlatformBackendBase::GetMainThreadId() const {
    return m_MainThreadId;
}

void PlatformBackendBase::SleepMilliseconds(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void PlatformBackendBase::YieldThread() {
    std::this_thread::yield();
}

SyncMutex* PlatformBackendBase::CreateMutex() { return new SyncMutex(); }
void PlatformBackendBase::DestroyMutex(SyncMutex* m) { delete m; }
void PlatformBackendBase::LockMutex(SyncMutex* m) { if (m) m->mutex.lock(); }
void PlatformBackendBase::UnlockMutex(SyncMutex* m) { if (m) m->mutex.unlock(); }

SyncEvent* PlatformBackendBase::CreateEvent(bool manualReset) {
    auto* e = new SyncEvent();
    e->manualReset = manualReset;
    return e;
}

void PlatformBackendBase::DestroyEvent(SyncEvent* e) { delete e; }

void PlatformBackendBase::SignalEvent(SyncEvent* e) {
    if (!e) return;
    {
        std::scoped_lock lock(e->mutex);
        e->signaled = true;
    }
    if (e->manualReset) {
        e->cv.notify_all();
    } else {
        e->cv.notify_one();
    }
}

void PlatformBackendBase::ResetEvent(SyncEvent* e) {
    if (!e) return;
    std::scoped_lock lock(e->mutex);
    e->signaled = false;
}

bool PlatformBackendBase::WaitEvent(SyncEvent* e, uint32_t timeoutMs) {
    if (!e) return false;
    std::unique_lock lock(e->mutex);
    if (timeoutMs == 0xFFFFFFFFu) {
        e->cv.wait(lock, [&] { return e->signaled; });
    } else {
        if (!e->cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&] { return e->signaled; })) {
            return false;
        }
    }
    if (!e->manualReset) {
        e->signaled = false;
    }
    return true;
}

std::string PlatformBackendBase::GetExecutablePath() const { return {}; }
std::string PlatformBackendBase::GetExecutableDirectory() const {
    return GetParentPath(GetExecutablePath());
}

std::string PlatformBackendBase::GetCurrentWorkingDirectory() const {
    std::error_code ec;
    return std::filesystem::current_path(ec).string();
}

bool PlatformBackendBase::SetCurrentWorkingDirectory(std::string_view path) {
    std::error_code ec;
    std::filesystem::current_path(path, ec);
    if (ec) {
        MakeError(PlatformErrorCode::OsFailure, ec.message(), "SetCurrentWorkingDirectory", ec.value());
        return false;
    }
    ClearLastError();
    return true;
}

std::string PlatformBackendBase::GetEnvironmentVariable(std::string_view) const { return {}; }
bool PlatformBackendBase::SetEnvironmentVariable(std::string_view, std::string_view) { return false; }
uint32_t PlatformBackendBase::GetProcessId() const { return 0; }
std::vector<std::string> PlatformBackendBase::GetCommandLineArgs() const { return {}; }

Result<ProcessLaunchResult> PlatformBackendBase::LaunchProcess(const ProcessLaunchDesc&) {
    return MakeError(PlatformErrorCode::NotSupported, "LaunchProcess not implemented.", "LaunchProcess");
}

Result<void> PlatformBackendBase::AddLibrarySearchPath(std::string_view) {
    return MakeError(PlatformErrorCode::NotSupported, "AddLibrarySearchPath not implemented.", "AddLibrarySearchPath");
}

Result<DynamicLibraryId> PlatformBackendBase::LoadLibrary(std::string_view) {
    return MakeError(PlatformErrorCode::NotSupported, "LoadLibrary not implemented.", "LoadLibrary");
}

Result<void> PlatformBackendBase::UnloadLibrary(DynamicLibraryId) {
    return Result<void>::Success();
}

void* PlatformBackendBase::GetLibrarySymbol(DynamicLibraryId, std::string_view) const { return nullptr; }

bool PlatformBackendBase::FileExists(std::string_view path) const {
    std::error_code ec;
    return std::filesystem::is_regular_file(path, ec);
}

bool PlatformBackendBase::DirectoryExists(std::string_view path) const {
    std::error_code ec;
    return std::filesystem::is_directory(path, ec);
}

bool PlatformBackendBase::CreateDirectories(std::string_view path) {
    std::error_code ec;
    return std::filesystem::create_directories(path, ec) || std::filesystem::is_directory(path, ec);
}

bool PlatformBackendBase::DeleteFile(std::string_view path) {
    std::error_code ec;
    return std::filesystem::remove(path, ec);
}

bool PlatformBackendBase::DeleteDirectory(std::string_view path) {
    std::error_code ec;
    return std::filesystem::remove_all(path, ec) > 0;
}

std::optional<std::vector<uint8_t>> PlatformBackendBase::ReadFileBytes(std::string_view path) const {
    std::ifstream file(std::string(path), std::ios::binary | std::ios::ate);
    if (!file) {
        return std::nullopt;
    }
    const auto size = file.tellg();
    if (size < 0) {
        return std::nullopt;
    }
    std::vector<uint8_t> data(static_cast<size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

bool PlatformBackendBase::WriteFileBytes(std::string_view path, std::span<const uint8_t> data) {
    std::ofstream file(std::string(path), std::ios::binary | std::ios::trunc);
    if (!file) {
        return false;
    }
    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return static_cast<bool>(file);
}

std::vector<std::string> PlatformBackendBase::ListDirectory(std::string_view path) const {
    std::vector<std::string> entries;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
        entries.push_back(entry.path().filename().string());
    }
    return entries;
}

std::string PlatformBackendBase::GetAbsolutePath(std::string_view path) const {
    std::error_code ec;
    return std::filesystem::absolute(path, ec).string();
}

std::string PlatformBackendBase::JoinPath(std::string_view a, std::string_view b) const {
    return (std::filesystem::path(a) / std::filesystem::path(b)).string();
}

std::string PlatformBackendBase::GetFileName(std::string_view path) const {
    return std::filesystem::path(path).filename().string();
}

std::string PlatformBackendBase::GetParentPath(std::string_view path) const {
    return std::filesystem::path(path).parent_path().string();
}

std::string PlatformBackendBase::GetExtension(std::string_view path) const {
    return std::filesystem::path(path).extension().string();
}

Result<DirectoryWatcherId> PlatformBackendBase::WatchDirectory(std::string_view, bool) {
    return MakeError(PlatformErrorCode::NotSupported, "Directory watching not implemented.", "WatchDirectory");
}

Result<void> PlatformBackendBase::UnwatchDirectory(DirectoryWatcherId) {
    return Result<void>::Success();
}

MemoryInfo PlatformBackendBase::GetMemoryInfo() const { return {}; }

CpuInfo PlatformBackendBase::GetCpuInfo() const {
    CpuInfo info;
    info.logicalCores = std::thread::hardware_concurrency();
    info.physicalCores = info.logicalCores;
    return info;
}

std::vector<GpuAdapterInfo> PlatformBackendBase::GetGpuAdapters() const { return {}; }
std::string PlatformBackendBase::GetLocale() const { return "en-US"; }
std::string PlatformBackendBase::GetTimeZone() const { return "UTC"; }
std::string PlatformBackendBase::GetComputerName() const { return {}; }
std::string PlatformBackendBase::GetUserName() const { return {}; }

void PlatformBackendBase::DebugOutput(std::string_view message) {
    WE_LOG_INFO(we::LogCategory::Diagnostics, std::string(message));
}

void PlatformBackendBase::DebugBreak() {
#if defined(_MSC_VER)
    __debugbreak();
#elif defined(__GNUC__) || defined(__clang__)
    __builtin_trap();
#endif
}

void PlatformBackendBase::SetCrashHandler(std::function<void(std::string_view)> handler) {
    m_CrashHandler = std::move(handler);
}

void PlatformBackendBase::RequestCrashDump(std::string_view reason) {
    if (m_CrashHandler) {
        m_CrashHandler(reason);
    }
    WE_LOG_ERROR(we::LogCategory::Crash, std::string("Crash requested: ") + std::string(reason));
}

bool PlatformBackendBase::AttachConsole() { return false; }

void PlatformBackendBase::ConsoleWrite(std::string_view text) {
    std::fwrite(text.data(), 1, text.size(), stdout);
}

} // namespace we::platform
