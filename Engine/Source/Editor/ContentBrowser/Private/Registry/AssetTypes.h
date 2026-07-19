#pragma once

#include <string>
#include <cstdint>

namespace we::editor::contentbrowser {

enum class AssetType {
    Unknown,
    Folder,
    Texture,
    Material,
    MaterialInstance,
    StaticMesh,
    SkeletalMesh,
    Animation,
    Blueprint,
    Scene,
    Prefab,
    Terrain,
    Audio,
    Font,
    Script,
    Video
};

inline std::string AssetTypeToString(AssetType type) {
    switch (type) {
        case AssetType::Folder: return "Folder";
        case AssetType::Texture: return "Texture";
        case AssetType::Material: return "Material";
        case AssetType::MaterialInstance: return "Material Instance";
        case AssetType::StaticMesh: return "Static Mesh";
        case AssetType::SkeletalMesh: return "Skeletal Mesh";
        case AssetType::Animation: return "Animation";
        case AssetType::Blueprint: return "Blueprint";
        case AssetType::Scene: return "Scene";
        case AssetType::Prefab: return "Prefab";
        case AssetType::Terrain: return "Terrain";
        case AssetType::Audio: return "Audio";
        case AssetType::Font: return "Font";
        case AssetType::Script: return "Script";
        case AssetType::Video: return "Video";
        default: return "Unknown";
    }
}

inline std::string AssetTypeToKey(AssetType type) {
    switch (type) {
        case AssetType::Folder: return "folder";
        case AssetType::Texture: return "texture";
        case AssetType::Material: return "material";
        case AssetType::MaterialInstance: return "material_instance";
        case AssetType::StaticMesh: return "static_mesh";
        case AssetType::SkeletalMesh: return "skeletal_mesh";
        case AssetType::Animation: return "animation";
        case AssetType::Blueprint: return "blueprint";
        case AssetType::Scene: return "scene";
        case AssetType::Prefab: return "prefab";
        case AssetType::Terrain: return "terrain";
        case AssetType::Audio: return "audio";
        case AssetType::Font: return "font";
        case AssetType::Script: return "script";
        case AssetType::Video: return "video";
        default: return "unknown";
    }
}

struct AssetRecord {
    std::string id;
    std::string name;
    std::string virtualPath;
    std::string diskPath;
    std::string parentPath;
    std::string extension;
    AssetType type = AssetType::Unknown;
    bool isFolder = false;
    bool isFavorite = false;
    bool isDirty = false;
    bool isReferenced = true;
    uint64_t modifiedTime = 0;
    uint64_t fileSize = 0;
    uint32_t contentVersion = 0;
};

} // namespace we::editor::contentbrowser
