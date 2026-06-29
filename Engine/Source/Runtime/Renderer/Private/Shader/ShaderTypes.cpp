#include "Renderer/Shader/ShaderTypes.hpp"

namespace we::runtime::renderer {

bool ShaderPermutationKey::operator==(const ShaderPermutationKey& other) const
{
    return shaderName == other.shaderName
        && stage == other.stage
        && permutationFlags == other.permutationFlags;
}

} // namespace we::runtime::renderer
