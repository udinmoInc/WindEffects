#pragma once

#include "Platform/IPlatform.h"
#include "Platform/Platform.h"
#include "Platform/Result.h"
#include "Platform/Types.h"

#include <utility>

namespace we::platform {

// Move-only RAII wrappers — destroy platform resources automatically.
// Prefer these over bare WindowId / DynamicLibraryId in engine code.

class PLATFORM_API ScopedWindow {
public:
    ScopedWindow() = default;
    explicit ScopedWindow(WindowId id) noexcept : m_Id(id) {}
    explicit ScopedWindow(const WindowDesc& desc);

    ScopedWindow(const ScopedWindow&) = delete;
    ScopedWindow& operator=(const ScopedWindow&) = delete;

    ScopedWindow(ScopedWindow&& other) noexcept : m_Id(other.m_Id) { other.m_Id = WindowId::Invalid; }
    ScopedWindow& operator=(ScopedWindow&& other) noexcept {
        if (this != &other) {
            Reset();
            m_Id = other.m_Id;
            other.m_Id = WindowId::Invalid;
        }
        return *this;
    }

    ~ScopedWindow() { Reset(); }

    void Reset() noexcept;
    [[nodiscard]] WindowId Release() noexcept {
        const WindowId id = m_Id;
        m_Id = WindowId::Invalid;
        return id;
    }

    [[nodiscard]] WindowId Get() const noexcept { return m_Id; }
    [[nodiscard]] explicit operator bool() const noexcept { return m_Id != WindowId::Invalid; }

private:
    WindowId m_Id = WindowId::Invalid;
};

class PLATFORM_API ScopedLibrary {
public:
    ScopedLibrary() = default;
    explicit ScopedLibrary(DynamicLibraryId id) noexcept : m_Id(id) {}
    explicit ScopedLibrary(std::string_view path);

    ScopedLibrary(const ScopedLibrary&) = delete;
    ScopedLibrary& operator=(const ScopedLibrary&) = delete;
    ScopedLibrary(ScopedLibrary&& other) noexcept : m_Id(other.m_Id) { other.m_Id = DynamicLibraryId::Invalid; }
    ScopedLibrary& operator=(ScopedLibrary&& other) noexcept {
        if (this != &other) {
            Reset();
            m_Id = other.m_Id;
            other.m_Id = DynamicLibraryId::Invalid;
        }
        return *this;
    }

    ~ScopedLibrary() { Reset(); }

    void Reset() noexcept;
    [[nodiscard]] DynamicLibraryId Get() const noexcept { return m_Id; }
    [[nodiscard]] void* GetSymbol(std::string_view name) const;
    [[nodiscard]] explicit operator bool() const noexcept { return m_Id != DynamicLibraryId::Invalid; }

private:
    DynamicLibraryId m_Id = DynamicLibraryId::Invalid;
};

class PLATFORM_API ScopedMutex {
public:
    ScopedMutex();
    ~ScopedMutex();
    ScopedMutex(const ScopedMutex&) = delete;
    ScopedMutex& operator=(const ScopedMutex&) = delete;

    void Lock();
    void Unlock();
    [[nodiscard]] SyncMutex* Get() const noexcept { return m_Mutex; }

private:
    SyncMutex* m_Mutex = nullptr;
};

class PLATFORM_API ScopedLock {
public:
    explicit ScopedLock(ScopedMutex& mutex) : m_Mutex(&mutex) { m_Mutex->Lock(); }
    ~ScopedLock() { if (m_Mutex) m_Mutex->Unlock(); }
    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;

private:
    ScopedMutex* m_Mutex = nullptr;
};

class PLATFORM_API ScopedDirectoryWatcher {
public:
    ScopedDirectoryWatcher() = default;
    explicit ScopedDirectoryWatcher(std::string_view path, bool recursive = true);
    ~ScopedDirectoryWatcher();

    ScopedDirectoryWatcher(const ScopedDirectoryWatcher&) = delete;
    ScopedDirectoryWatcher& operator=(const ScopedDirectoryWatcher&) = delete;
    ScopedDirectoryWatcher(ScopedDirectoryWatcher&& other) noexcept : m_Id(other.m_Id) {
        other.m_Id = DirectoryWatcherId::Invalid;
    }
    ScopedDirectoryWatcher& operator=(ScopedDirectoryWatcher&& other) noexcept {
        if (this != &other) {
            Reset();
            m_Id = other.m_Id;
            other.m_Id = DirectoryWatcherId::Invalid;
        }
        return *this;
    }

    void Reset() noexcept;
    [[nodiscard]] DirectoryWatcherId Get() const noexcept { return m_Id; }
    [[nodiscard]] explicit operator bool() const noexcept { return m_Id != DirectoryWatcherId::Invalid; }

private:
    DirectoryWatcherId m_Id = DirectoryWatcherId::Invalid;
};

} // namespace we::platform
