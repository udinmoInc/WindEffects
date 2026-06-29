#pragma once

#include "Renderer/Shader/ShaderReflection.hpp"
#include "Renderer/Shader/ShaderTypes.hpp"
#include <future>
#include <string>

namespace we::runtime::renderer {

class ShaderCompiler {
public:
    ShaderBytecode LoadSPIRVFromDisk(const std::string& filename) const;
    std::future<ShaderBytecode> CompileAsync(const ShaderCompileRequest& request) const;

    // Runtime DXC invocation for editor hot-reload (SPIR-V output).
    ShaderBytecode CompileSPIRV(const ShaderCompileRequest& request) const;
};

} // namespace we::runtime::renderer
