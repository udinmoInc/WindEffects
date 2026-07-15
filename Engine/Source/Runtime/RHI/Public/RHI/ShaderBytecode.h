#pragma once

#include "RHI/Export.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace we::rhi {

// Backend-neutral compiled shader asset loader.
// Looks under Engine/Shaders/Bytecodes and Assets/Shaders for
//   {shaderName}_{VS|PS|CS}{.spv|.dxil|.metallib|.glsl}
class RHI_API ShaderBytecodeLoader {
public:
    [[nodiscard]] static ShaderBytecodeFormat DetectFormat(std::span<const uint8_t> bytecode) noexcept;

    [[nodiscard]] static ShaderBytecodeFormat ResolveFormat(
        IRHIDevice* device,
        ShaderBytecodeFormat requested = ShaderBytecodeFormat::Auto) noexcept;

    // Load one compiled stage. stageToken is "VS", "PS", "CS", etc.
    [[nodiscard]] static std::vector<uint8_t> Load(
        std::string_view shaderName,
        std::string_view stageToken,
        ShaderBytecodeFormat format);
};

} // namespace we::rhi
