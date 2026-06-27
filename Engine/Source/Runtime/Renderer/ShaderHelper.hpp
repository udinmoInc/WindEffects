#pragma once

#include <volk.h>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <iostream>

namespace HouseEngine {

inline std::vector<char> ReadShaderFile(const std::string& filename) {
    std::ifstream file;
    
    std::vector<std::string> searchPaths = {
        filename,
        "Shaders/" + filename,
        "../Shaders/" + filename,
        "Build/bin/Shaders/" + filename,
        "bin/Shaders/" + filename,
        "../Build/bin/Shaders/" + filename,
        "../../Shaders/" + filename
    };

    for (const auto& path : searchPaths) {
        file.open(path, std::ios::ate | std::ios::binary);
        if (file.is_open()) {
            break;
        }
        file.clear();
    }

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filename);
    }

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

} // namespace HouseEngine
