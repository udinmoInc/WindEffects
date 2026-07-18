#pragma once

#include "Core/EditorConfigPaths.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace we::programs::editor {
namespace first_run_detail {

inline constexpr float kDialogPadding = 16.0f;
inline constexpr float kButtonHeight = 32.0f;
inline constexpr float kButtonWidth = 110.0f;
inline constexpr float kButtonGap = 10.0f;
inline constexpr float kCopyButtonWidth = 110.0f;
inline constexpr float kScrollbarWidth = 12.0f;
inline constexpr float kScrollbarMinThumbSize = 30.0f;
inline constexpr float kSmoothScrollSpeed = 0.15f;

inline constexpr float kBaseFontSize = 13.0f;
inline constexpr float kHeading1FontSize = 18.0f;
inline constexpr float kHeading2FontSize = 15.0f;
inline constexpr float kHeading3FontSize = 13.0f;
inline constexpr float kCodeFontSize = 12.0f;
inline constexpr float kBlockquoteFontSize = 13.0f;

inline constexpr float kBaseLineHeight = 1.45f;
inline constexpr float kCodeLineHeight = 1.4f;

inline constexpr float kParagraphSpacing = 8.0f;
inline constexpr float kHeadingSpacing = 10.0f;
inline constexpr float kListSpacing = 4.0f;
inline constexpr float kCodeBlockPadding = 10.0f;
inline constexpr float kBlockquotePadding = 10.0f;
inline constexpr float kContentMargin = 16.0f;

inline std::filesystem::path GetAgreementStatePath() {
    return we::core::ResolveEditorConfigPath("first_run_agreement.ini");
}

inline std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return {};
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

inline std::filesystem::path ResolveRuntimeFile(const std::string& relativePath) {
    const std::filesystem::path p(relativePath);
    if (std::filesystem::exists(p)) return p;
    const std::filesystem::path candidates[] = {
        std::filesystem::path("..") / relativePath,
        std::filesystem::path("../..") / relativePath
    };
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) return candidate;
    }
    return {};
}

inline bool StartsWith(const std::string& str, const std::string& prefix) {
    return str.rfind(prefix, 0) == 0;
}

inline std::string Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

inline std::string TrimLeft(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    return str.substr(start);
}

} // namespace first_run_detail
} // namespace we::programs::editor
