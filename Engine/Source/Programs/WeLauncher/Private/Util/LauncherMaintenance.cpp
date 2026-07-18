#include "Util/LauncherMaintenance.h"

#include "Util/PathUtils.h"

#include <filesystem>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Shell32.lib")
#include "Platform/UndefWin32Macros.h"
#endif

namespace we::programs::welauncher {
namespace {

#if defined(_WIN32)

std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(
        CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (size <= 0) {
        return {};
    }
    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), size);
    return wide;
}

bool SetRegistryString(HKEY root, const wchar_t* subKey, const wchar_t* valueName, const std::wstring& value) {
    HKEY key = nullptr;
    const LONG create = RegCreateKeyExW(
        root,
        subKey,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE | KEY_WRITE,
        nullptr,
        &key,
        nullptr);
    if (create != ERROR_SUCCESS || !key) {
        return false;
    }
    const LONG set = RegSetValueExW(
        key,
        valueName,
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(value.c_str()),
        static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
    return set == ERROR_SUCCESS;
}

#endif

} // namespace

AssociationResult AssociateProjectExtension(const std::string& extensionWithDot) {
    AssociationResult result;
    if (extensionWithDot.empty() || extensionWithDot.front() != '.') {
        result.message = "Invalid extension";
        return result;
    }

#if defined(_WIN32)
    // Double-click opens the Editor with the project path (Unreal-style).
    const auto exeDir = PathUtils::GetExecutableDirectory();
    const std::filesystem::path exePath = exeDir / "WindeffectsEditor.exe";
    std::error_code ec;
    if (!std::filesystem::exists(exePath, ec)) {
        result.message = "WindeffectsEditor.exe not found next to the running process";
        return result;
    }

    const std::wstring ext = Utf8ToWide(extensionWithDot);
    const std::wstring progId = L"WindEffects.Project" + ext;
    const std::wstring exeWide = exePath.wstring();
    const std::wstring command = L"\"" + exeWide + L"\" \"%1\"";
    const std::wstring friendly = L"WindEffects Project";

    if (!SetRegistryString(HKEY_CURRENT_USER, (L"Software\\Classes\\" + ext).c_str(), nullptr, progId)) {
        result.message = "Failed to register file extension";
        return result;
    }
    if (!SetRegistryString(HKEY_CURRENT_USER, (L"Software\\Classes\\" + progId).c_str(), nullptr, friendly)) {
        result.message = "Failed to register ProgID";
        return result;
    }
    if (!SetRegistryString(
            HKEY_CURRENT_USER,
            (L"Software\\Classes\\" + progId + L"\\shell\\open\\command").c_str(),
            nullptr,
            command)) {
        result.message = "Failed to register open command";
        return result;
    }

    result.ok = true;
    result.message = "Associated " + extensionWithDot + " with WindeffectsEditor";
    return result;
#else
    result.message = "File associations are only supported on Windows";
    return result;
#endif
}

bool OpenPathInExplorer(const std::string& pathUtf8) {
    if (pathUtf8.empty()) {
        return false;
    }
#if defined(_WIN32)
    const auto path = PathUtils::FromUtf8(pathUtf8);
    std::error_code ec;
    std::filesystem::create_directories(
        std::filesystem::is_directory(path, ec) ? path : path.parent_path(),
        ec);
    const std::wstring wide = path.wstring();
    return reinterpret_cast<INT_PTR>(ShellExecuteW(
               nullptr,
               L"explore",
               wide.c_str(),
               nullptr,
               nullptr,
               SW_SHOWNORMAL)) > 32
        || reinterpret_cast<INT_PTR>(ShellExecuteW(
               nullptr,
               L"open",
               wide.c_str(),
               nullptr,
               nullptr,
               SW_SHOWNORMAL)) > 32;
#else
    (void)pathUtf8;
    return false;
#endif
}

bool OpenUrl(const std::string& url) {
    if (url.empty()) {
        return false;
    }
#if defined(_WIN32)
    const std::wstring wide = Utf8ToWide(url);
    return reinterpret_cast<INT_PTR>(ShellExecuteW(
               nullptr,
               L"open",
               wide.c_str(),
               nullptr,
               nullptr,
               SW_SHOWNORMAL)) > 32;
#else
    (void)url;
    return false;
#endif
}

} // namespace we::programs::welauncher
