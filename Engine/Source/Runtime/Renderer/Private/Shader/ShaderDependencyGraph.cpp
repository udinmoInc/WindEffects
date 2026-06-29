#include "Renderer/Shader/ShaderDependencyGraph.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <unordered_set>

namespace we::runtime::renderer {

namespace {

std::string NormalizePath(const std::filesystem::path& p)
{
    return p.generic_string();
}

void CollectIncludesRecursive(const std::filesystem::path& file, const std::filesystem::path& shaderRoot,
                              std::vector<std::string>& out, std::unordered_set<std::string>& visited)
{
    const std::string key = NormalizePath(file);
    if (!visited.insert(key).second)
        return;

    std::ifstream stream(file);
    if (!stream)
        return;

    std::string line;
    while (std::getline(stream, line))
    {
        const auto pos = line.find("#include");
        if (pos == std::string::npos)
            continue;

        const auto firstQuote = line.find('"', pos);
        if (firstQuote == std::string::npos)
            continue;
        const auto secondQuote = line.find('"', firstQuote + 1);
        if (secondQuote == std::string::npos)
            continue;

        const std::string includeName = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
        std::filesystem::path includePath = file.parent_path() / includeName;
        if (!std::filesystem::exists(includePath))
            includePath = shaderRoot / includeName;

        if (std::filesystem::exists(includePath))
        {
            out.push_back(NormalizePath(includePath));
            CollectIncludesRecursive(includePath, shaderRoot, out, visited);
        }
    }
}

} // namespace

void ShaderDependencyGraph::ScanIncludeTree(const std::filesystem::path& shaderRoot)
{
    m_ShaderRoot = shaderRoot;
    m_ShaderToIncludes.clear();
    m_IncludeToShaders.clear();

    if (!std::filesystem::exists(shaderRoot))
        return;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(shaderRoot))
    {
        if (!entry.is_regular_file())
            continue;
        if (entry.path().extension() != ".hlsl")
            continue;

        const std::string shaderPath = NormalizePath(entry.path());
        std::vector<std::string> includes;
        std::unordered_set<std::string> visited;
        CollectIncludesRecursive(entry.path(), shaderRoot, includes, visited);
        RegisterShader(shaderPath, includes);
    }
}

void ShaderDependencyGraph::RegisterShader(const std::string& shaderPath, const std::vector<std::string>& includes)
{
    m_ShaderToIncludes[shaderPath] = includes;
    for (const auto& inc : includes)
        m_IncludeToShaders[inc].push_back(shaderPath);
}

std::vector<std::string> ShaderDependencyGraph::GetDependents(const std::string& includePath) const
{
    const auto it = m_IncludeToShaders.find(includePath);
    return it != m_IncludeToShaders.end() ? it->second : std::vector<std::string>{};
}

std::vector<std::string> ShaderDependencyGraph::GetIncludes(const std::string& shaderPath) const
{
    const auto it = m_ShaderToIncludes.find(shaderPath);
    return it != m_ShaderToIncludes.end() ? it->second : std::vector<std::string>{};
}

uint64_t ShaderDependencyGraph::GetLatestTimestamp(const std::string& shaderPath) const
{
    uint64_t latest = 0;
    auto touch = [&latest](const std::string& path) {
        if (!std::filesystem::exists(path))
            return;
        const auto t = std::filesystem::last_write_time(path);
        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count();
        latest = std::max(latest, static_cast<uint64_t>(ns));
    };

    touch(shaderPath);
    for (const auto& inc : GetIncludes(shaderPath))
        touch(inc);

    return latest;
}

} // namespace we::runtime::renderer
