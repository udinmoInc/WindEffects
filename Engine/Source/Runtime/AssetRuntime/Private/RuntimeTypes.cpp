#include "AssetRuntime/RuntimeTypes.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace we::runtime::assetruntime {
namespace {

std::string ToLowerExt(std::string_view extension) {
    std::string ext(extension);
    if (!ext.empty() && ext.front() != '.') {
        ext.insert(ext.begin(), '.');
    }
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return ext;
}

} // namespace

bool IsCookedAssetExtension(std::string_view extension) {
    const std::string ext = ToLowerExt(extension);
    return ext == ".weasset" || ext == ".wefont" || ext == ".weiconatlas" || ext == ".wetex"
        || ext == ".wemesh" || ext == ".weanim" || ext == ".weaudio" || ext == ".weshader"
        || ext == ".wemat" || ext == ".wescene" || ext == ".wemeta";
}

bool IsRawSourceExtension(std::string_view extension) {
    const std::string ext = ToLowerExt(extension);
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp"
        || ext == ".hdr" || ext == ".exr" || ext == ".webp" || ext == ".svg" || ext == ".ttf"
        || ext == ".otf" || ext == ".ttc" || ext == ".fbx" || ext == ".obj" || ext == ".gltf"
        || ext == ".glb" || ext == ".dae" || ext == ".wav" || ext == ".mp3" || ext == ".ogg"
        || ext == ".flac" || ext == ".mp4" || ext == ".avi" || ext == ".mov" || ext == ".webm"
        || ext == ".hlsl" || ext == ".glsl";
}

} // namespace we::runtime::assetruntime
