#pragma once

#include "Registry/AssetTypes.h"
#include <algorithm>
#include <cctype>
#include <string>

namespace we::editor::contentbrowser {

class AssetTypeResolver {
public:
    static AssetType FromExtension(const std::string& ext) {
        std::string lower = ext;
        std::transform(lower.begin(), lower.end(), lower.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (lower == ".png" || lower == ".jpg" || lower == ".jpeg" || lower == ".tga" ||
            lower == ".bmp" || lower == ".svg" || lower == ".webp" || lower == ".hdr") {
            return AssetType::Texture;
        }
        if (lower == ".mat") return AssetType::Material;
        if (lower == ".matinst" || lower == ".mi") return AssetType::MaterialInstance;
        if (lower == ".fbx" || lower == ".obj" || lower == ".gltf" || lower == ".glb" || lower == ".mesh") {
            return AssetType::StaticMesh;
        }
        if (lower == ".skel" || lower == ".skeleton") return AssetType::SkeletalMesh;
        if (lower == ".anim" || lower == ".animation") return AssetType::Animation;
        if (lower == ".bp" || lower == ".blueprint") return AssetType::Blueprint;
        if (lower == ".scene" || lower == ".level" || lower == ".umap") return AssetType::Scene;
        if (lower == ".prefab") return AssetType::Prefab;
        if (lower == ".wav" || lower == ".mp3" || lower == ".ogg" || lower == ".flac") return AssetType::Audio;
        if (lower == ".ttf" || lower == ".otf" || lower == ".woff" || lower == ".woff2") return AssetType::Font;
        if (lower == ".lua" || lower == ".cs" || lower == ".py" || lower == ".js" || lower == ".ts") return AssetType::Script;
        if (lower == ".mp4" || lower == ".avi" || lower == ".mov" || lower == ".webm") return AssetType::Video;
        return AssetType::Unknown;
    }

    static AssetType FromPath(const std::string& path, bool isDirectory) {
        if (isDirectory) return AssetType::Folder;
        const auto dot = path.find_last_of('.');
        if (dot == std::string::npos) return AssetType::Unknown;
        return FromExtension(path.substr(dot));
    }
};

} // namespace we::editor::contentbrowser
