#include "AssetRuntime/VirtualPath.h"

#include <cctype>

namespace we::runtime::assetruntime {
namespace {

bool IsAllowedPathChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '/' || c == '_' || c == '-'
        || c == '.' || c == ' ';
}

} // namespace

VirtualPath::VirtualPath(std::string path)
    : m_Path(Normalize(path).Get()) {}

VirtualPath::VirtualPath(std::string_view path)
    : m_Path(Normalize(path).Get()) {}

VirtualPath::VirtualPath(const char* path)
    : m_Path(path ? Normalize(path).Get() : std::string{}) {}

bool VirtualPath::IsValid() const noexcept {
    return !m_Path.empty() && m_Path.front() == '/';
}

std::string_view VirtualPath::MountRoot() const noexcept {
    if (m_Path.size() < 2 || m_Path.front() != '/') {
        return {};
    }
    const auto second = m_Path.find('/', 1);
    if (second == std::string::npos) {
        return m_Path;
    }
    return std::string_view(m_Path).substr(0, second);
}

std::string_view VirtualPath::Relative() const noexcept {
    const auto root = MountRoot();
    if (root.empty() || m_Path.size() <= root.size()) {
        return {};
    }
    // Skip leading slash after root
    size_t start = root.size();
    if (start < m_Path.size() && m_Path[start] == '/') {
        ++start;
    }
    return std::string_view(m_Path).substr(start);
}

VirtualPath VirtualPath::Normalize(std::string_view path) {
    if (path.empty()) {
        return VirtualPath();
    }

    std::string out;
    out.reserve(path.size());
    out.push_back('/');

    for (char c : path) {
        if (c == '\\') {
            c = '/';
        }
        if (!IsAllowedPathChar(c)) {
            return VirtualPath();
        }
        if (c == '/') {
            if (out.back() == '/') {
                continue;
            }
            out.push_back('/');
            continue;
        }
        out.push_back(c);
    }

    while (out.size() > 1 && out.back() == '/') {
        out.pop_back();
    }

    // Must start with /Name…
    if (out.size() < 2) {
        return VirtualPath();
    }
    VirtualPath result;
    result.m_Path = std::move(out);
    return result;
}

std::size_t VirtualPathHash::operator()(const VirtualPath& path) const noexcept {
    return std::hash<std::string>{}(path.Get());
}

} // namespace we::runtime::assetruntime
