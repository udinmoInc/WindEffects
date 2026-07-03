#include "Core/IgniteBTInvoker.hpp"
#include "Core/BuildPaths.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace we::core {
namespace {

std::string JsonReadString(const std::string& json, const char* key) {
    const std::string needle = std::string("\"") + key + "\"";
    const auto keyPos = json.find(needle);
    if (keyPos == std::string::npos) {
        return {};
    }

    const auto colonPos = json.find(':', keyPos + needle.size());
    if (colonPos == std::string::npos) {
        return {};
    }

    const auto quoteStart = json.find('"', colonPos + 1);
    if (quoteStart == std::string::npos) {
        return {};
    }

    const auto quoteEnd = json.find('"', quoteStart + 1);
    if (quoteEnd == std::string::npos) {
        return {};
    }

    return json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
}

std::filesystem::path GetInstallManifestPath() {
#if defined(_WIN32)
    if (const char* localAppData = std::getenv("LOCALAPPDATA")) {
        return std::filesystem::path(localAppData) / "WindEffects" / "engine.json";
    }
    return {};
#else
    if (const char* configHome = std::getenv("XDG_CONFIG_HOME")) {
        return std::filesystem::path(configHome) / "windeffects" / "engine.json";
    }

    if (const char* home = std::getenv("HOME")) {
        return std::filesystem::path(home) / ".config" / "windeffects" / "engine.json";
    }

    return {};
#endif
}

bool TryReadInstallManifest(std::filesystem::path& outProjectRoot) {
    const auto manifestPath = GetInstallManifestPath();
    if (manifestPath.empty() || !std::filesystem::exists(manifestPath)) {
        return false;
    }

    std::ifstream input(manifestPath);
    if (!input) {
        return false;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    const std::string projectRoot = JsonReadString(buffer.str(), "ProjectRoot");
    if (projectRoot.empty()) {
        return false;
    }

    outProjectRoot = projectRoot;
    return std::filesystem::exists(outProjectRoot / "Engine" / "Source");
}

bool TryResolveProjectRoot(std::filesystem::path& outProjectRoot) {
    if (const char* envRoot = std::getenv("WE_PROJECT_ROOT")) {
        outProjectRoot = envRoot;
        if (std::filesystem::exists(outProjectRoot / "Engine" / "Source")) {
            return true;
        }
    }

    if (TryReadInstallManifest(outProjectRoot)) {
        return true;
    }

    auto dir = GetExecutableDirectory();
    while (!dir.empty()) {
        if (std::filesystem::exists(dir / "Engine" / "Source")) {
            outProjectRoot = dir;
            return true;
        }

        if (!dir.has_parent_path() || dir.parent_path() == dir) {
            break;
        }

        dir = dir.parent_path();
    }

    return false;
}

std::string GetIgniteBTBinaryName() {
#if defined(_WIN32)
    return "IgniteBT.exe";
#else
    return "IgniteBT";
#endif
}

} // namespace

bool TryResolveIgniteBTExecutable(std::string& outExecutablePath, std::string& outWorkingDirectory) {
    std::filesystem::path projectRoot;
    if (!TryResolveProjectRoot(projectRoot)) {
        return false;
    }

    outWorkingDirectory = projectRoot.string();
    const char* configurations[] = { "Debug", "Release", "Development", "Shipping" };
    for (const char* config : configurations) {
        const auto candidate = projectRoot
            / "Build" / "Intermediate" / "IgniteBT" / config / "net8.0" / GetIgniteBTBinaryName();
        if (std::filesystem::exists(candidate)) {
            outExecutablePath = candidate.string();
            return true;
        }
    }

    return false;
}

IgniteBTInvokeResult InvokeIgniteBT(const std::vector<std::string>& args) {
    IgniteBTInvokeResult result;

    std::string executable;
    std::string workingDirectory;
    if (!TryResolveIgniteBTExecutable(executable, workingDirectory)) {
        result.errorMessage = "IgniteBT executable was not found. Build IgniteBT or run `we setup`.";
        return result;
    }

#if defined(_WIN32)
    std::string commandLine = "\"" + executable + "\"";
    for (const auto& arg : args) {
        commandLine.push_back(' ');
        commandLine.push_back('"');
        for (char ch : arg) {
            if (ch == '"') {
                commandLine.append("\\\"");
            } else {
                commandLine.push_back(ch);
            }
        }
        commandLine.push_back('"');
    }

    std::vector<char> mutableCommandLine(commandLine.begin(), commandLine.end());
    mutableCommandLine.push_back('\0');

    STARTUPINFOA startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};

    if (!CreateProcessA(
            nullptr,
            mutableCommandLine.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NEW_CONSOLE,
            nullptr,
            workingDirectory.c_str(),
            &startupInfo,
            &processInfo)) {
        result.errorMessage = "Failed to launch IgniteBT.";
        return result;
    }

    CloseHandle(processInfo.hThread);
    WaitForSingleObject(processInfo.hProcess, INFINITE);

    DWORD exitCode = 1;
    GetExitCodeProcess(processInfo.hProcess, &exitCode);
    CloseHandle(processInfo.hProcess);

    result.launched = true;
    result.exitCode = static_cast<int>(exitCode);
    return result;
#else
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(executable.c_str()));
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        if (chdir(workingDirectory.c_str()) != 0) {
            _exit(127);
        }
        execv(executable.c_str(), argv.data());
        _exit(127);
    }

    if (pid < 0) {
        result.errorMessage = "Failed to launch IgniteBT.";
        return result;
    }

    int status = 0;
    waitpid(pid, &status, 0);
    result.launched = true;
    result.exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    return result;
#endif
}

} // namespace we::core
