#include "Shader/ShaderPermutationManager.h"
#include <sstream>

namespace we::runtime::renderer {

std::vector<std::string> ShaderPermutationManager::BuildDefines(uint32_t permutationFlags) const
{
    std::vector<std::string> defines;
    if (permutationFlags & kForwardPlus)
        defines.emplace_back("WE_FORWARD_PLUS=1");
    if (permutationFlags & kDeferred)
        defines.emplace_back("WE_DEFERRED=1");
    if (permutationFlags & kBindless)
        defines.emplace_back("WE_BINDLESS=1");
    if (permutationFlags & kVolumetricFog)
        defines.emplace_back("WE_VOLUMETRIC_FOG=1");
    return defines;
}

std::string ShaderPermutationManager::BuildPermutationSuffix(uint32_t permutationFlags) const
{
    if (permutationFlags == 0)
        return {};

    std::ostringstream oss;
    oss << "_P" << std::hex << permutationFlags;
    return oss.str();
}

} // namespace we::runtime::renderer
