#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::renderer {

enum class ShaderStage : uint8_t {
    Vertex = 0,
    Pixel = 1,
    Compute = 2,
    Mesh = 3,
    Amplification = 4
};

enum class ShaderTargetApi : uint8_t {
    SPIRV = 0,
    DXIL = 1,
    Metal = 2
};

struct ShaderPermutationKey {
    std::string shaderName;
    ShaderStage stage = ShaderStage::Vertex;
    uint32_t permutationFlags = 0;

    bool operator==(const ShaderPermutationKey& other) const;
};

struct ShaderCompileRequest {
    std::string sourcePath;
    std::string entryPoint = "main";
    ShaderStage stage = ShaderStage::Vertex;
    ShaderTargetApi targetApi = ShaderTargetApi::SPIRV;
    uint32_t permutationFlags = 0;
    std::vector<std::string> defines;
};

} // namespace we::runtime::renderer
