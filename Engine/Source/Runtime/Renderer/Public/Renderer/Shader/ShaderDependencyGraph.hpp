#pragma once

#include "Renderer/Shader/ShaderTypes.hpp"
#include <filesystem>
#include <unordered_map>
#include <vector>

namespace we::runtime::renderer {

// Tracks #include dependencies for incremental and hot reload.
class ShaderDependencyGraph {
public:
    void ScanIncludeTree(const std::filesystem::path& shaderRoot);
    void RegisterShader(const std::string& shaderPath, const std::vector<std::string>& includes);

    std::vector<std::string> GetDependents(const std::string& includePath) const;
    std::vector<std::string> GetIncludes(const std::string& shaderPath) const;

    uint64_t GetLatestTimestamp(const std::string& shaderPath) const;

private:
    std::unordered_map<std::string, std::vector<std::string>> m_ShaderToIncludes;
    std::unordered_map<std::string, std::vector<std::string>> m_IncludeToShaders;
    std::filesystem::path m_ShaderRoot;
};

} // namespace we::runtime::renderer
