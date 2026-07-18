#pragma once

#include "AssetRuntime/Export.h"

#include <string>
#include <string_view>

namespace we::runtime::assetruntime {

/// Virtual asset path (e.g. "/Game/Textures/Hero", "/Engine/Fonts/Default", "/DLC1/Meshes/Boss").
/// Never a filesystem path — resolved through mounted package virtual roots.
class ASSETRUNTIME_API VirtualPath {
public:
    VirtualPath() = default;
    explicit VirtualPath(std::string path);
    VirtualPath(std::string_view path);
    VirtualPath(const char* path);

    [[nodiscard]] const std::string& Get() const noexcept { return m_Path; }
    [[nodiscard]] bool Empty() const noexcept { return m_Path.empty(); }
    [[nodiscard]] bool IsValid() const noexcept;

    [[nodiscard]] std::string_view MountRoot() const noexcept;
    [[nodiscard]] std::string_view Relative() const noexcept;

    [[nodiscard]] bool operator==(const VirtualPath& other) const noexcept {
        return m_Path == other.m_Path;
    }
    [[nodiscard]] bool operator!=(const VirtualPath& other) const noexcept {
        return m_Path != other.m_Path;
    }

    /// Normalize separators and collapse redundant slashes. Returns empty on invalid input.
    [[nodiscard]] static VirtualPath Normalize(std::string_view path);

private:
    std::string m_Path;
};

struct ASSETRUNTIME_API VirtualPathHash {
    [[nodiscard]] std::size_t operator()(const VirtualPath& path) const noexcept;
};

} // namespace we::runtime::assetruntime
