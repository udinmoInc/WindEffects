#pragma once

#include "Renderer/Shader/ShaderTypes.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace we::runtime::renderer {

// Manages shader permutation defines (PBR variants, forward+/deferred, etc.).
class ShaderPermutationManager {
public:
    static constexpr uint32_t kForwardPlus = 1u << 0;
    static constexpr uint32_t kDeferred = 1u << 1;
    static constexpr uint32_t kBindless = 1u << 2;
    static constexpr uint32_t kVolumetricFog = 1u << 3;

    std::vector<std::string> BuildDefines(uint32_t permutationFlags) const;
    std::string BuildPermutationSuffix(uint32_t permutationFlags) const;
};

} // namespace we::runtime::renderer
