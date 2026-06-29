#pragma once

#include "Renderer/Shader/ShaderLibrary.hpp"
#include "Renderer/Shader/ShaderTypes.hpp"
#include <volk.h>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <iostream>

namespace we::runtime::renderer {

inline std::vector<char> LoadShaderBytecode(const std::string& shaderName, ShaderStage stage, uint32_t permutationFlags = 0)
{
    const ShaderBytecode bytecode = ShaderLibrary::Get().GetBytecode(shaderName, stage, permutationFlags);
    if (bytecode.data.empty())
    {
        throw std::runtime_error(
            "Failed to load shader bytecode: " + shaderName
            + ShaderLibrary::Get().ResolveBytecodeFilename(shaderName, stage, permutationFlags));
    }

    std::cout << "[Shader] Loaded " << shaderName
              << ShaderLibrary::Get().ResolveBytecodeFilename(shaderName, stage, permutationFlags).substr(shaderName.size())
              << " (" << bytecode.data.size() << " bytes)\n";

    return std::vector<char>(bytecode.data.begin(), bytecode.data.end());
}

inline const char* ShaderStageEntryPoint(ShaderStage stage)
{
    switch (stage)
    {
    case ShaderStage::Vertex: return "VSMain";
    case ShaderStage::Pixel: return "PSMain";
    case ShaderStage::Compute: return "CSMain";
    default: return "main";
    }
}

// Legacy helper — prefer LoadShaderBytecode(shaderName, stage).
inline std::vector<char> ReadShaderFile(const std::string& filename) {
    std::ifstream file;
    std::string resolvedPath;

    std::vector<std::string> searchPaths = {
        "Assets/Shaders/" + filename,
        "Shaders/" + filename,
        "../Assets/Shaders/" + filename,
        "../Shaders/" + filename,
        filename
    };

    for (const auto& path : searchPaths) {
        file.open(path, std::ios::ate | std::ios::binary);
        if (file.is_open()) {
            resolvedPath = path;
            break;
        }
        file.clear();
    }

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filename + " (searched Assets/Shaders/, Shaders/, and cwd)");
    }

    std::cout << "[Shader] Loaded " << filename << " from " << resolvedPath << " (" << file.tellg() << " bytes)\n";

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

inline VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shaderModule;
}

} // namespace we::runtime::renderer
