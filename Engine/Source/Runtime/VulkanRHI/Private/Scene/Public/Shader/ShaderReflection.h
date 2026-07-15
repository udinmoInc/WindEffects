#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::renderer {

struct ShaderBytecode {
    std::vector<uint8_t> data;
    std::string debugName;
    uint64_t sourceTimestamp = 0;
};

struct ShaderReflectionBinding {
    uint32_t set = 0;
    uint32_t binding = 0;
    std::string name;
};

struct ShaderReflection {
    std::string entryPoint;
    std::vector<ShaderReflectionBinding> bindings;
    uint32_t pushConstantSize = 0;
};

} // namespace we::runtime::renderer
