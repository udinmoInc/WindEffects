#include "RHI/ShaderBytecode.h"

#include "Core/Paths.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace we::rhi {
namespace {

std::vector<uint8_t> ReadFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return {};
    }
    const auto size = static_cast<std::streamsize>(file.tellg());
    if (size <= 0) {
        return {};
    }
    std::vector<uint8_t> data(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(data.data()), size);
    return file ? data : std::vector<uint8_t>{};
}

} // namespace

ShaderBytecodeFormat ShaderBytecodeLoader::DetectFormat(std::span<const uint8_t> bytecode) noexcept {
    if (bytecode.size() >= 4) {
        const uint32_t magic = static_cast<uint32_t>(bytecode[0])
            | (static_cast<uint32_t>(bytecode[1]) << 8)
            | (static_cast<uint32_t>(bytecode[2]) << 16)
            | (static_cast<uint32_t>(bytecode[3]) << 24);
        if (magic == 0x07230203u) {
            return ShaderBytecodeFormat::SpirV;
        }
        if (bytecode[0] == 'D' && bytecode[1] == 'X' && bytecode[2] == 'B' && bytecode[3] == 'C') {
            return ShaderBytecodeFormat::Dxil;
        }
    }
    return ShaderBytecodeFormat::Auto;
}

ShaderBytecodeFormat ShaderBytecodeLoader::ResolveFormat(
    IRHIDevice* device,
    ShaderBytecodeFormat requested) noexcept
{
    if (requested != ShaderBytecodeFormat::Auto) {
        return requested;
    }
    if (device) {
        return PreferredShaderBytecodeFormat(device->GetBackend());
    }
    return ShaderBytecodeFormat::SpirV;
}

std::vector<uint8_t> ShaderBytecodeLoader::Load(
    std::string_view shaderName,
    std::string_view stageToken,
    ShaderBytecodeFormat format)
{
    if (format == ShaderBytecodeFormat::Auto) {
        format = ShaderBytecodeFormat::SpirV;
    }
    const char* ext = ShaderBytecodeExtension(format);
    if (!ext || !*ext) {
        return {};
    }

    const std::string fileName = std::string(shaderName) + "_" + std::string(stageToken) + ext;
    for (const auto& candidate : we::core::PathService::Get().ShaderBytecodeCandidates(fileName)) {
        auto data = ReadFile(candidate);
        if (!data.empty()) {
            return data;
        }
    }
    return {};
}

} // namespace we::rhi
