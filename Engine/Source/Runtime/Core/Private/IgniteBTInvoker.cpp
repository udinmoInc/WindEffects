#include "Core/IgniteBTInvoker.h"
#include "Core/BuildPaths.h"
#include "Core/Environment.h"
#include "Core/Paths.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
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
    return PathService::Get().UserDataRoot() / "engine.json";
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

    outProjectRoot = PathService::FromUtf8(projectRoot);
    std::error_code ec;
    return std::filesystem::exists(outProjectRoot / layout::kEngine / layout::kSource, ec);
}

bool TryResolveProjectRoot(std::filesystem::path& outProjectRoot) {
    if (auto envRoot = GetEnvVar("WE_PROJECT_ROOT")) {
        if (auto found = PathService::FindRepositoryRoot(PathService::FromUtf8(*envRoot))) {
            outProjectRoot = *found;
            return true;
        }
    }
    if (auto envRoot = GetEnvVar("WE_ENGINE_ROOT")) {
        if (auto found = PathService::FindRepositoryRoot(PathService::FromUtf8(*envRoot))) {
            outProjectRoot = *found;
            return true;
        }
    }

    if (TryReadInstallManifest(outProjectRoot)) {
        return true;
    }

    if (auto found = PathService::FindRepositoryRoot(PathService::Get().ExecutableDirectory())) {
        outProjectRoot = *found;
        return true;
    }

    return false;
}

std::optional<std::filesystem::path> FindIgniteBTUnderRoot(const std::filesystem::path& projectRoot) {
#if defined(_WIN32)
    const char* binaryName = "IgniteBT.exe";
#else
    const char* binaryName = "IgniteBT";
#endif
    const char* configurations[] = { "Debug", "Release", "Development", "Shipping" };
    std::vector<std::filesystem::path> candidates;
    candidates.reserve(4);
    for (const char* config : configurations) {
        candidates.push_back(
            projectRoot / layout::kBuild / layout::kIntermediate / "IgniteBT" / config / "net8.0" / binaryName);
    }
    return PathService::FindExisting(candidates);
}

} // namespace

bool TryResolveIgniteBTExecutable(std::string& outExecutablePath, std::string& outWorkingDirectory) {
    std::filesystem::path projectRoot;
    if (!TryResolveProjectRoot(projectRoot)) {
        return false;
    }

    const auto candidate = FindIgniteBTUnderRoot(projectRoot);
    if (!candidate) {
        return false;
    }

    outWorkingDirectory = PathService::ToUtf8(projectRoot);
    outExecutablePath = PathService::ToUtf8(*candidate);
    return true;
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
    argv.push_back(executable.data());
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
