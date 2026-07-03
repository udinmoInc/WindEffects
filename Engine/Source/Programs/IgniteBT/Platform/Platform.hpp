#pragma once

#include "Core/Types.hpp"

namespace IgniteBT {

// Platform information
struct PlatformInfo {
    Platform platform = Platform::Unknown;
    Architecture architecture = Architecture::Unknown;
    std::string osVersion;
    std::string toolchainVersion;
};

// Platform manager interface
class IPlatform {
public:
    virtual ~IPlatform() = default;
    
    virtual PlatformInfo GetPlatformInfo() const = 0;
    virtual Path GetExecutablePath() const = 0;
    virtual Path GetHomeDirectory() const = 0;
    virtual Path GetTempDirectory() const = 0;
    virtual Path GetCurrentDirectory() const = 0;
    virtual bool SetCurrentDirectory(const Path& path) = 0;
    
    virtual bool FileExists(const Path& path) const = 0;
    virtual bool DirectoryExists(const Path& path) const = 0;
    virtual bool CreateDirectory(const Path& path) = 0;
    virtual bool CreateDirectories(const Path& path) = 0;
    virtual bool DeleteFile(const Path& path) = 0;
    virtual bool DeleteDirectory(const Path& path, bool recursive = false) = 0;
    
    virtual std::vector<Path> ListFiles(const Path& directory, const std::string& pattern = "*") const = 0;
    virtual std::vector<Path> ListDirectories(const Path& directory) const = 0;
    
    virtual Timestamp GetFileModificationTime(const Path& path) const = 0;
    virtual uint64_t GetFileSize(const Path& path) const = 0;
    
    virtual std::string ExecuteCommand(const std::string& command) const = 0;
    virtual int ExecuteCommand(const std::string& command, std::string& output) const = 0;
};

// Platform manager
class PlatformManager {
public:
    static PlatformManager& Get();
    
    IPlatform* GetPlatform();
    const PlatformInfo& GetPlatformInfo() const;
    
    Platform GetCurrentPlatform() const;
    Architecture GetCurrentArchitecture() const;
    
    bool IsWindows() const;
    bool IsLinux() const;
    bool IsMac() const;
    
    bool Is64Bit() const;
    
private:
    PlatformManager();
    ~PlatformManager() = default;
    
    std::unique_ptr<IPlatform> m_platform;
    PlatformInfo m_platformInfo;
};

// Windows platform implementation
#ifdef _WIN32
class WindowsPlatform : public IPlatform {
public:
    PlatformInfo GetPlatformInfo() const override;
    Path GetExecutablePath() const override;
    Path GetHomeDirectory() const override;
    Path GetTempDirectory() const override;
    Path GetCurrentDirectory() const override;
    bool SetCurrentDirectory(const Path& path) override;
    
    bool FileExists(const Path& path) const override;
    bool DirectoryExists(const Path& path) const override;
    bool CreateDirectory(const Path& path) override;
    bool CreateDirectories(const Path& path) override;
    bool DeleteFile(const Path& path) override;
    bool DeleteDirectory(const Path& path, bool recursive) override;
    
    std::vector<Path> ListFiles(const Path& directory, const std::string& pattern) const override;
    std::vector<Path> ListDirectories(const Path& directory) const override;
    
    Timestamp GetFileModificationTime(const Path& path) const override;
    uint64_t GetFileSize(const Path& path) const override;
    
    std::string ExecuteCommand(const std::string& command) const override;
    int ExecuteCommand(const std::string& command, std::string& output) const override;
};
#endif

// Linux platform implementation
#ifdef __linux__
class LinuxPlatform : public IPlatform {
public:
    PlatformInfo GetPlatformInfo() const override;
    Path GetExecutablePath() const override;
    Path GetHomeDirectory() const override;
    Path GetTempDirectory() const override;
    Path GetCurrentDirectory() const override;
    bool SetCurrentDirectory(const Path& path) override;
    
    bool FileExists(const Path& path) const override;
    bool DirectoryExists(const Path& path) const override;
    bool CreateDirectory(const Path& path) override;
    bool CreateDirectories(const Path& path) override;
    bool DeleteFile(const Path& path) override;
    bool DeleteDirectory(const Path& path, bool recursive) override;
    
    std::vector<Path> ListFiles(const Path& directory, const std::string& pattern) const override;
    std::vector<Path> ListDirectories(const Path& directory) const override;
    
    Timestamp GetFileModificationTime(const Path& path) const override;
    uint64_t GetFileSize(const Path& path) const override;
    
    std::string ExecuteCommand(const std::string& command) const override;
    int ExecuteCommand(const std::string& command, std::string& output) const override;
};
#endif

// Mac platform implementation
#ifdef __APPLE__
class MacPlatform : public IPlatform {
public:
    PlatformInfo GetPlatformInfo() const override;
    Path GetExecutablePath() const override;
    Path GetHomeDirectory() const override;
    Path GetTempDirectory() const override;
    Path GetCurrentDirectory() const override;
    bool SetCurrentDirectory(const Path& path) override;
    
    bool FileExists(const Path& path) const override;
    bool DirectoryExists(const Path& path) const override;
    bool CreateDirectory(const Path& path) override;
    bool CreateDirectories(const Path& path) override;
    bool DeleteFile(const Path& path) override;
    bool DeleteDirectory(const Path& path, bool recursive) override;
    
    std::vector<Path> ListFiles(const Path& directory, const std::string& pattern) const override;
    std::vector<Path> ListDirectories(const Path& directory) const override;
    
    Timestamp GetFileModificationTime(const Path& path) const override;
    uint64_t GetFileSize(const Path& path) const override;
    
    std::string ExecuteCommand(const std::string& command) const override;
    int ExecuteCommand(const std::string& command, std::string& output) const override;
};
#endif

} // namespace IgniteBT
