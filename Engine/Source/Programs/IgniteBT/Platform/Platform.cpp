#include "Platform/Platform.hpp"
#include "Logging/Logger.hpp"
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
// Undefine Windows macros that conflict with our method names
#undef GetCurrentDirectory
#undef SetCurrentDirectory
#undef CreateDirectory
#undef DeleteFile
#undef RemoveDirectory
#undef GetFileAttributes
#undef FindFirstFile
#undef FindNextFile
#undef FindClose
#endif

namespace IgniteBT {

PlatformManager& PlatformManager::Get() {
    static PlatformManager instance;
    return instance;
}

PlatformManager::PlatformManager() {
#ifdef _WIN32
    m_platform = std::make_unique<WindowsPlatform>();
#elif __linux__
    m_platform = std::make_unique<LinuxPlatform>();
#elif __APPLE__
    m_platform = std::make_unique<MacPlatform>();
#else
    LOG_ERROR("Unsupported platform", "Platform");
    m_platform = nullptr;
#endif
    
    if (m_platform) {
        m_platformInfo = m_platform->GetPlatformInfo();
    }
}

IPlatform* PlatformManager::GetPlatform() {
    return m_platform.get();
}

const PlatformInfo& PlatformManager::GetPlatformInfo() const {
    return m_platformInfo;
}

Platform PlatformManager::GetCurrentPlatform() const {
    return m_platformInfo.platform;
}

Architecture PlatformManager::GetCurrentArchitecture() const {
    return m_platformInfo.architecture;
}

bool PlatformManager::IsWindows() const {
    return m_platformInfo.platform == Platform::Windows;
}

bool PlatformManager::IsLinux() const {
    return m_platformInfo.platform == Platform::Linux;
}

bool PlatformManager::IsMac() const {
    return m_platformInfo.platform == Platform::Mac;
}

bool PlatformManager::Is64Bit() const {
    return m_platformInfo.architecture == Architecture::x64 || 
           m_platformInfo.architecture == Architecture::ARM64;
}

#ifdef _WIN32
PlatformInfo WindowsPlatform::GetPlatformInfo() const {
    PlatformInfo info;
    info.platform = Platform::Windows;
    info.architecture = Architecture::x64;
    
    // Get OS version
    OSVERSIONINFOEXA osvi;
    ::ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    
    // Note: GetVersionEx is deprecated but still works for basic version info
    // For production, use VersionHelpers or RtlGetVersion
    info.osVersion = "Windows";
    
    // Get architecture
    SYSTEM_INFO si;
    ::GetNativeSystemInfo(&si);
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
        info.architecture = Architecture::x64;
    } else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) {
        info.architecture = Architecture::ARM64;
    }
    
    return info;
}

Path WindowsPlatform::GetExecutablePath() const {
    char buffer[MAX_PATH];
    ::GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return Path(buffer).parent_path();
}

Path WindowsPlatform::GetHomeDirectory() const {
    char buffer[MAX_PATH];
    if (::SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, buffer) == S_OK) {
        return Path(buffer);
    }
    return Path();
}

Path WindowsPlatform::GetTempDirectory() const {
    char buffer[MAX_PATH];
    ::GetTempPathA(MAX_PATH, buffer);
    return Path(buffer);
}

Path WindowsPlatform::GetCurrentDirectory() const {
    char buffer[MAX_PATH];
    DWORD result = ::GetCurrentDirectoryA(MAX_PATH, buffer);
    (void)result;
    return Path(buffer);
}

bool WindowsPlatform::SetCurrentDirectory(const Path& path) {
    return ::SetCurrentDirectoryA(path.string().c_str()) != FALSE;
}

bool WindowsPlatform::FileExists(const Path& path) const {
    DWORD attrs = ::GetFileAttributesA(path.string().c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool WindowsPlatform::DirectoryExists(const Path& path) const {
    DWORD attrs = ::GetFileAttributesA(path.string().c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool WindowsPlatform::CreateDirectory(const Path& path) {
    return ::CreateDirectoryA(path.string().c_str(), NULL) != FALSE;
}

bool WindowsPlatform::CreateDirectories(const Path& path) {
    return std::filesystem::create_directories(path);
}

bool WindowsPlatform::DeleteFile(const Path& path) {
    return ::DeleteFileA(path.string().c_str()) != FALSE;
}

bool WindowsPlatform::DeleteDirectory(const Path& path, bool recursive) {
    if (recursive) {
        return std::filesystem::remove_all(path) > 0;
    }
    return ::RemoveDirectoryA(path.string().c_str()) != FALSE;
}

std::vector<Path> WindowsPlatform::ListFiles(const Path& directory, const std::string& pattern) const {
    std::vector<Path> files;
    std::string searchPattern = (directory / pattern).string();
    
    WIN32_FIND_DATAA findData;
    HANDLE hFind = ::FindFirstFileA(searchPattern.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                files.push_back(directory / findData.cFileName);
            }
        } while (::FindNextFileA(hFind, &findData));
        ::FindClose(hFind);
    }
    
    return files;
}

std::vector<Path> WindowsPlatform::ListDirectories(const Path& directory) const {
    std::vector<Path> dirs;
    std::string searchPattern = (directory / "*").string();
    
    WIN32_FIND_DATAA findData;
    HANDLE hFind = ::FindFirstFileA(searchPattern.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && 
                strcmp(findData.cFileName, ".") != 0 && 
                strcmp(findData.cFileName, "..") != 0) {
                dirs.push_back(directory / findData.cFileName);
            }
        } while (::FindNextFileA(hFind, &findData));
        ::FindClose(hFind);
    }
    
    return dirs;
}

Timestamp WindowsPlatform::GetFileModificationTime(const Path& path) const {
    WIN32_FILE_ATTRIBUTE_DATA fileData;
    if (::GetFileAttributesExA(path.string().c_str(), ::GetFileExInfoStandard, &fileData)) {
        ULARGE_INTEGER ull;
        ull.LowPart = fileData.ftLastWriteTime.dwLowDateTime;
        ull.HighPart = fileData.ftLastWriteTime.dwHighDateTime;
        
        // Convert FILETIME to milliseconds since epoch
        uint64_t fileTime = ull.QuadPart;
        uint64_t epoch = 116444736000000000ULL; // January 1, 1970 in FILETIME
        uint64_t milliseconds = (fileTime - epoch) / 10000;
        
        auto duration = std::chrono::milliseconds(milliseconds);
        return Timestamp(duration);
    }
    return Timestamp();
}

uint64_t WindowsPlatform::GetFileSize(const Path& path) const {
    WIN32_FILE_ATTRIBUTE_DATA fileData;
    if (::GetFileAttributesExA(path.string().c_str(), ::GetFileExInfoStandard, &fileData)) {
        LARGE_INTEGER size;
        size.LowPart = fileData.nFileSizeLow;
        size.HighPart = fileData.nFileSizeHigh;
        return static_cast<uint64_t>(size.QuadPart);
    }
    return 0;
}

std::string WindowsPlatform::ExecuteCommand(const std::string& command) const {
    std::string output;
    ExecuteCommand(command, output);
    return output;
}

int WindowsPlatform::ExecuteCommand(const std::string& command, std::string& output) const {
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) return -1;
    
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        output += buffer;
    }
    
    return _pclose(pipe);
}
#endif

#ifdef __linux__
PlatformInfo LinuxPlatform::GetPlatformInfo() const {
    PlatformInfo info;
    info.platform = Platform::Linux;
    info.architecture = Architecture::x64;
    
    // Get architecture from uname
    FILE* pipe = popen("uname -m", "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe)) {
            std::string arch = buffer;
            if (arch.find("x86_64") != std::string::npos) {
                info.architecture = Architecture::x64;
            } else if (arch.find("aarch64") != std::string::npos) {
                info.architecture = Architecture::ARM64;
            }
        }
        pclose(pipe);
    }
    
    // Get OS version
    pipe = popen("lsb_release -ds 2>/dev/null || cat /etc/os-release | head -n1", "r");
    if (pipe) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), pipe)) {
            info.osVersion = buffer;
            // Remove trailing newline
            if (!info.osVersion.empty() && info.osVersion.back() == '\n') {
                info.osVersion.pop_back();
            }
        }
        pclose(pipe);
    }
    
    return info;
}

Path LinuxPlatform::GetExecutablePath() const {
    char buffer[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", buffer, PATH_MAX);
    if (count != -1) {
        buffer[count] = '\0';
        return Path(buffer).parent_path();
    }
    return Path();
}

Path LinuxPlatform::GetHomeDirectory() const {
    const char* home = getenv("HOME");
    return home ? Path(home) : Path();
}

Path LinuxPlatform::GetTempDirectory() const {
    const char* tmp = getenv("TMPDIR");
    return tmp ? Path(tmp) : Path("/tmp");
}

Path LinuxPlatform::GetCurrentDirectory() const {
    char buffer[PATH_MAX];
    if (getcwd(buffer, PATH_MAX)) {
        return Path(buffer);
    }
    return Path();
}

bool LinuxPlatform::SetCurrentDirectory(const Path& path) const {
    return chdir(path.string().c_str()) == 0;
}

bool LinuxPlatform::FileExists(const Path& path) const {
    struct stat buffer;
    return stat(path.string().c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode);
}

bool LinuxPlatform::DirectoryExists(const Path& path) const {
    struct stat buffer;
    return stat(path.string().c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode);
}

bool LinuxPlatform::CreateDirectory(const Path& path) const {
    return mkdir(path.string().c_str(), 0755) == 0;
}

bool LinuxPlatform::CreateDirectories(const Path& path) const {
    return std::filesystem::create_directories(path);
}

bool LinuxPlatform::DeleteFile(const Path& path) const {
    return unlink(path.string().c_str()) == 0;
}

bool LinuxPlatform::DeleteDirectory(const Path& path, bool recursive) const {
    if (recursive) {
        return std::filesystem::remove_all(path) > 0;
    }
    return rmdir(path.string().c_str()) == 0;
}

std::vector<Path> LinuxPlatform::ListFiles(const Path& directory, const std::string& pattern) const {
    std::vector<Path> files;
    
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            if (pattern == "*" || entry.path().filename().string().find(pattern) != std::string::npos) {
                files.push_back(entry.path());
            }
        }
    }
    
    return files;
}

std::vector<Path> LinuxPlatform::ListDirectories(const Path& directory) const {
    std::vector<Path> dirs;
    
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_directory()) {
            dirs.push_back(entry.path());
        }
    }
    
    return dirs;
}

Timestamp LinuxPlatform::GetFileModificationTime(const Path& path) const {
    struct stat buffer;
    if (stat(path.string().c_str(), &buffer) == 0) {
        auto duration = std::chrono::milliseconds(buffer.st_mtime * 1000);
        return Timestamp(duration);
    }
    return Timestamp();
}

uint64_t LinuxPlatform::GetFileSize(const Path& path) const {
    struct stat buffer;
    if (stat(path.string().c_str(), &buffer) == 0) {
        return static_cast<uint64_t>(buffer.st_size);
    }
    return 0;
}

std::string LinuxPlatform::ExecuteCommand(const std::string& command) const {
    std::string output;
    ExecuteCommand(command, output);
    return output;
}

int LinuxPlatform::ExecuteCommand(const std::string& command, std::string& output) const {
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return -1;
    
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        output += buffer;
    }
    
    return pclose(pipe);
}
#endif

#ifdef __APPLE__
PlatformInfo MacPlatform::GetPlatformInfo() const {
    PlatformInfo info;
    info.platform = Platform::Mac;
    info.architecture = Architecture::x64;
    
    // Get architecture
    FILE* pipe = popen("uname -m", "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe)) {
            std::string arch = buffer;
            if (arch.find("x86_64") != std::string::npos) {
                info.architecture = Architecture::x64;
            } else if (arch.find("arm64") != std::string::npos) {
                info.architecture = Architecture::ARM64;
            }
        }
        pclose(pipe);
    }
    
    // Get OS version
    pipe = popen("sw_vers -productVersion", "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe)) {
            info.osVersion = "macOS ";
            info.osVersion += buffer;
            if (!info.osVersion.empty() && info.osVersion.back() == '\n') {
                info.osVersion.pop_back();
            }
        }
        pclose(pipe);
    }
    
    return info;
}

Path MacPlatform::GetExecutablePath() const {
    char buffer[PATH_MAX];
    uint32_t bufsize = PATH_MAX;
    if (_NSGetExecutablePath(buffer, &bufsize) == 0) {
        return Path(buffer).parent_path();
    }
    return Path();
}

Path MacPlatform::GetHomeDirectory() const {
    const char* home = getenv("HOME");
    return home ? Path(home) : Path();
}

Path MacPlatform::GetTempDirectory() const {
    const char* tmp = getenv("TMPDIR");
    return tmp ? Path(tmp) : Path("/tmp");
}

Path MacPlatform::GetCurrentDirectory() const {
    char buffer[PATH_MAX];
    if (getcwd(buffer, PATH_MAX)) {
        return Path(buffer);
    }
    return Path();
}

bool MacPlatform::SetCurrentDirectory(const Path& path) const {
    return chdir(path.string().c_str()) == 0;
}

bool MacPlatform::FileExists(const Path& path) const {
    struct stat buffer;
    return stat(path.string().c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode);
}

bool MacPlatform::DirectoryExists(const Path& path) const {
    struct stat buffer;
    return stat(path.string().c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode);
}

bool MacPlatform::CreateDirectory(const Path& path) const {
    return mkdir(path.string().c_str(), 0755) == 0;
}

bool MacPlatform::CreateDirectories(const Path& path) const {
    return std::filesystem::create_directories(path);
}

bool MacPlatform::DeleteFile(const Path& path) const {
    return unlink(path.string().c_str()) == 0;
}

bool MacPlatform::DeleteDirectory(const Path& path, bool recursive) const {
    if (recursive) {
        return std::filesystem::remove_all(path) > 0;
    }
    return rmdir(path.string().c_str()) == 0;
}

std::vector<Path> MacPlatform::ListFiles(const Path& directory, const std::string& pattern) const {
    std::vector<Path> files;
    
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            if (pattern == "*" || entry.path().filename().string().find(pattern) != std::string::npos) {
                files.push_back(entry.path());
            }
        }
    }
    
    return files;
}

std::vector<Path> MacPlatform::ListDirectories(const Path& directory) const {
    std::vector<Path> dirs;
    
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_directory()) {
            dirs.push_back(entry.path());
        }
    }
    
    return dirs;
}

Timestamp MacPlatform::GetFileModificationTime(const Path& path) const {
    struct stat buffer;
    if (stat(path.string().c_str(), &buffer) == 0) {
        auto duration = std::chrono::milliseconds(buffer.st_mtime * 1000);
        return Timestamp(duration);
    }
    return Timestamp();
}

uint64_t MacPlatform::GetFileSize(const Path& path) const {
    struct stat buffer;
    if (stat(path.string().c_str(), &buffer) == 0) {
        return static_cast<uint64_t>(buffer.st_size);
    }
    return 0;
}

std::string MacPlatform::ExecuteCommand(const std::string& command) const {
    std::string output;
    ExecuteCommand(command, output);
    return output;
}

int MacPlatform::ExecuteCommand(const std::string& command, std::string& output) const {
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return -1;
    
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        output += buffer;
    }
    
    return pclose(pipe);
}
#endif

} // namespace IgniteBT
