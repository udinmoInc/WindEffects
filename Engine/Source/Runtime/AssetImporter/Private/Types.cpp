#include "AssetImporter/Types.h"

#include <algorithm>
#include <cctype>

namespace we::runtime::assetimporter {
namespace {

std::string ToLower(std::string_view text) {
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

} // namespace

std::string_view AssetKindToString(AssetKind kind) {
    switch (kind) {
    case AssetKind::Texture: return "Texture";
    case AssetKind::Font: return "Font";
    case AssetKind::Icon: return "Icon";
    case AssetKind::IconAtlas: return "IconAtlas";
    case AssetKind::StaticMesh: return "StaticMesh";
    case AssetKind::SkeletalMesh: return "SkeletalMesh";
    case AssetKind::Animation: return "Animation";
    case AssetKind::Audio: return "Audio";
    case AssetKind::Shader: return "Shader";
    case AssetKind::Material: return "Material";
    case AssetKind::MaterialInstance: return "MaterialInstance";
    case AssetKind::Scene: return "Scene";
    case AssetKind::Blueprint: return "Blueprint";
    case AssetKind::Prefab: return "Prefab";
    case AssetKind::Script: return "Script";
    case AssetKind::Video: return "Video";
    case AssetKind::RawBinary: return "RawBinary";
    default: return "Unknown";
    }
}

AssetKind AssetKindFromString(std::string_view name) {
    const std::string lower = ToLower(name);
    if (lower == "texture") return AssetKind::Texture;
    if (lower == "font") return AssetKind::Font;
    if (lower == "icon") return AssetKind::Icon;
    if (lower == "iconatlas" || lower == "icon_atlas") return AssetKind::IconAtlas;
    if (lower == "staticmesh" || lower == "static_mesh" || lower == "mesh") return AssetKind::StaticMesh;
    if (lower == "skeletalmesh" || lower == "skeletal_mesh") return AssetKind::SkeletalMesh;
    if (lower == "animation" || lower == "anim") return AssetKind::Animation;
    if (lower == "audio") return AssetKind::Audio;
    if (lower == "shader") return AssetKind::Shader;
    if (lower == "material") return AssetKind::Material;
    if (lower == "materialinstance" || lower == "material_instance") return AssetKind::MaterialInstance;
    if (lower == "scene" || lower == "level") return AssetKind::Scene;
    if (lower == "blueprint" || lower == "bp") return AssetKind::Blueprint;
    if (lower == "prefab") return AssetKind::Prefab;
    if (lower == "script") return AssetKind::Script;
    if (lower == "video") return AssetKind::Video;
    if (lower == "rawbinary" || lower == "raw" || lower == "binary") return AssetKind::RawBinary;
    return AssetKind::Unknown;
}

std::string_view AssetKindNativeExtension(AssetKind kind) {
    switch (kind) {
    case AssetKind::Font: return ".wefont";
    case AssetKind::IconAtlas: return ".weiconatlas";
    case AssetKind::Texture: return ".wetex";
    case AssetKind::StaticMesh:
    case AssetKind::SkeletalMesh: return ".wemesh";
    case AssetKind::Animation: return ".weanim";
    case AssetKind::Audio: return ".weaudio";
    case AssetKind::Shader: return ".weshader";
    case AssetKind::Material:
    case AssetKind::MaterialInstance: return ".wemat";
    case AssetKind::Scene: return ".wescene";
    default: return ".weasset";
    }
}

AssetKind AssetKindFromSourceExtension(std::string_view extension) {
    std::string ext = ToLower(extension);
    if (!ext.empty() && ext.front() != '.') {
        ext.insert(ext.begin(), '.');
    }

    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp"
        || ext == ".hdr" || ext == ".exr" || ext == ".webp") {
        return AssetKind::Texture;
    }
    if (ext == ".svg") return AssetKind::Icon;
    if (ext == ".ttf" || ext == ".otf" || ext == ".ttc") return AssetKind::Font;
    if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".dae") {
        return AssetKind::StaticMesh;
    }
    if (ext == ".anim" || ext == ".animation") return AssetKind::Animation;
    if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac") return AssetKind::Audio;
    if (ext == ".hlsl" || ext == ".glsl" || ext == ".spv" || ext == ".shader") return AssetKind::Shader;
    if (ext == ".mat") return AssetKind::Material;
    if (ext == ".matinst" || ext == ".mi") return AssetKind::MaterialInstance;
    if (ext == ".scene" || ext == ".level" || ext == ".umap") return AssetKind::Scene;
    if (ext == ".bp" || ext == ".blueprint") return AssetKind::Blueprint;
    if (ext == ".prefab" || ext == ".weprefab") return AssetKind::Prefab;
    if (ext == ".lua" || ext == ".cs" || ext == ".py" || ext == ".js" || ext == ".ts") return AssetKind::Script;
    if (ext == ".mp4" || ext == ".avi" || ext == ".mov" || ext == ".webm") return AssetKind::Video;
    if (ext == ".atlas") return AssetKind::IconAtlas;
    return AssetKind::Unknown;
}

} // namespace we::runtime::assetimporter
