#include "Platform/Scoped.h"

#include "Platform/ThreadSafety.h"

namespace we::platform {

ScopedWindow::ScopedWindow(const WindowDesc& desc) {
    if (!Platform::IsAvailable()) {
        return;
    }
    AssertMainThread("ScopedWindow");
    auto result = Platform::Get().CreateWindow(desc);
    if (result) {
        m_Id = *result;
    }
}

void ScopedWindow::Reset() noexcept {
    if (m_Id == WindowId::Invalid || !Platform::IsAvailable()) {
        m_Id = WindowId::Invalid;
        return;
    }
    (void)Platform::Get().DestroyWindow(m_Id);
    m_Id = WindowId::Invalid;
}

ScopedLibrary::ScopedLibrary(std::string_view path) {
    if (!Platform::IsAvailable()) {
        return;
    }
    auto result = Platform::Get().LoadLibrary(path);
    if (result) {
        m_Id = *result;
    }
}

void ScopedLibrary::Reset() noexcept {
    if (m_Id == DynamicLibraryId::Invalid || !Platform::IsAvailable()) {
        m_Id = DynamicLibraryId::Invalid;
        return;
    }
    (void)Platform::Get().UnloadLibrary(m_Id);
    m_Id = DynamicLibraryId::Invalid;
}

void* ScopedLibrary::GetSymbol(std::string_view name) const {
    if (m_Id == DynamicLibraryId::Invalid || !Platform::IsAvailable()) {
        return nullptr;
    }
    return Platform::Get().GetLibrarySymbol(m_Id, name);
}

ScopedMutex::ScopedMutex() {
    if (Platform::IsAvailable()) {
        m_Mutex = Platform::Get().CreateMutex();
    }
}

ScopedMutex::~ScopedMutex() {
    if (m_Mutex && Platform::IsAvailable()) {
        Platform::Get().DestroyMutex(m_Mutex);
        m_Mutex = nullptr;
    }
}

void ScopedMutex::Lock() {
    if (m_Mutex && Platform::IsAvailable()) {
        Platform::Get().LockMutex(m_Mutex);
    }
}

void ScopedMutex::Unlock() {
    if (m_Mutex && Platform::IsAvailable()) {
        Platform::Get().UnlockMutex(m_Mutex);
    }
}

ScopedDirectoryWatcher::ScopedDirectoryWatcher(std::string_view path, bool recursive) {
    if (!Platform::IsAvailable()) {
        return;
    }
    auto result = Platform::Get().WatchDirectory(path, recursive);
    if (result) {
        m_Id = *result;
    }
}

ScopedDirectoryWatcher::~ScopedDirectoryWatcher() {
    Reset();
}

void ScopedDirectoryWatcher::Reset() noexcept {
    if (m_Id == DirectoryWatcherId::Invalid || !Platform::IsAvailable()) {
        m_Id = DirectoryWatcherId::Invalid;
        return;
    }
    (void)Platform::Get().UnwatchDirectory(m_Id);
    m_Id = DirectoryWatcherId::Invalid;
}

} // namespace we::platform
