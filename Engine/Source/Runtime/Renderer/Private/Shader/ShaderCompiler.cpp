#include "Renderer/Shader/ShaderCompiler.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

ShaderBytecode ShaderCompiler::LoadSPIRVFromDisk(const std::string& filename) const
{
    ShaderBytecode bytecode;
    bytecode.debugName = filename;

    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file)
        return bytecode;

    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    bytecode.data.resize(static_cast<size_t>(size));
    if (size > 0)
        file.read(reinterpret_cast<char*>(bytecode.data.data()), size);

    if (std::filesystem::exists(filename))
    {
        const auto t = std::filesystem::last_write_time(filename);
        bytecode.sourceTimestamp = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count());
    }

    return bytecode;
}

std::future<ShaderBytecode> ShaderCompiler::CompileAsync(const ShaderCompileRequest& request) const
{
    return std::async(std::launch::async, [this, request]() {
        return CompileSPIRV(request);
    });
}

ShaderBytecode ShaderCompiler::CompileSPIRV(const ShaderCompileRequest& request) const
{
    // Runtime DXC hot-reload path — build system is the primary compiler.
    // When DXC is unavailable, callers fall back to prebuilt SPIR-V on disk.
    (void)request;
    ShaderBytecode empty;
    empty.debugName = request.sourcePath;
    return empty;
}

#endif // WE_HAS_VULKAN

} // namespace we::runtime::renderer
