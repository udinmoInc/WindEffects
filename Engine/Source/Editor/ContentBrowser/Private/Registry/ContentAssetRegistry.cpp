#include "Registry/ContentAssetRegistry.hpp"
#include "Registry/AssetTypeResolver.hpp"
#include "Core/Logger.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace we::editor::contentbrowser {

ContentAssetRegistry& ContentAssetRegistry::Get() {
    static ContentAssetRegistry instance;
    return instance;
}

void ContentAssetRegistry::Initialize(const std::string& contentRoot) {
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_ContentRoot = contentRoot;
    }

    EnsureDemoContent();

    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        Refresh();
        m_Initialized = true;
    }

    HE_INFO("[ContentAssetRegistry] Initialized at: " + m_ContentRoot);
}

void ContentAssetRegistry::Shutdown() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Assets.clear();
    m_IdIndex.clear();
    m_PathIndex.clear();
    m_FolderVersions.clear();
    m_Initialized = false;
}

void ContentAssetRegistry::EnsureDemoContent() {
    const fs::path gameRoot = fs::path(m_ContentRoot) / "Game";
    if (fs::exists(gameRoot) && !fs::is_empty(gameRoot)) {
        return;
    }

    HE_INFO("[ContentAssetRegistry] Seeding demo content at: " + gameRoot.string());
    const auto makeDir = [&](const std::string& rel) {
        fs::create_directories(gameRoot / rel);
    };

    makeDir("Textures");
    makeDir("Materials");
    makeDir("Meshes");
    makeDir("Blueprints");
    makeDir("Audio");
    makeDir("Fonts");
    makeDir("Scripts");
    makeDir("Animations");
    makeDir("Maps");

    const fs::path iconRoot = "Engine/Content/Icons/icons";
    const auto copyIcon = [&](const std::string& iconName, const std::string& destRel) {
        const fs::path src = iconRoot / (iconName + ".svg");
        const fs::path dst = gameRoot / destRel;
        if (fs::exists(src)) {
            std::error_code ec;
            fs::create_directories(dst.parent_path(), ec);
            fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
        }
    };

    copyIcon("layers", "Textures/T_Brick_D.svg");
    copyIcon("image", "Textures/T_Grass_D.svg");
    copyIcon("palette", "Textures/T_Noise.svg");
    copyIcon("sparkles", "Textures/T_Sparkle.svg");

    copyIcon("cube", "Meshes/SM_Crate.obj");
    copyIcon("box", "Meshes/SM_Barrel.obj");
    copyIcon("cylinder", "Meshes/SM_Pillar.obj");

    copyIcon("workflow", "Blueprints/BP_Player.bp");
    copyIcon("component", "Blueprints/BP_GameMode.bp");

    copyIcon("music", "Audio/SFX_Click.wav");
    copyIcon("volume-2", "Audio/Music_MainTheme.mp3");

    copyIcon("type", "Fonts/UI_Font.ttf");

    copyIcon("file-code", "Scripts/GameMode.lua");
    copyIcon("terminal", "Scripts/PlayerController.cs");

    copyIcon("play", "Animations/Anim_Idle.anim");
    copyIcon("person-standing", "Animations/Anim_Walk.anim");

    copyIcon("map", "Maps/Level_01.scene");
    copyIcon("map-pin", "Maps/Level_02.scene");

    auto writeText = [&](const std::string& rel, const std::string& text) {
        const fs::path dst = gameRoot / rel;
        std::error_code ec;
        fs::create_directories(dst.parent_path(), ec);
        std::ofstream out(dst);
        out << text;
    };

    writeText("Materials/M_Concrete.mat", R"({"type":"Material","baseColor":[0.55,0.55,0.55,1.0],"roughness":0.8})");
    writeText("Materials/M_Metal.mat", R"({"type":"Material","baseColor":[0.7,0.72,0.78,1.0],"metallic":1.0,"roughness":0.3})");
    writeText("Materials/M_Glass.mat", R"({"type":"Material","baseColor":[0.9,0.95,1.0,0.3],"roughness":0.05})");
    writeText("Materials/MI_Concrete_Wet.matinst", R"({"type":"MaterialInstance","parent":"M_Concrete","roughness":0.2})");

    writeText("Meshes/SM_Crate.obj", "# Simple crate placeholder\nv -0.5 -0.5 -0.5\nv 0.5 0.5 0.5\nf 1 1 1\n");
    writeText("Meshes/SM_Barrel.obj", "# Barrel placeholder\nv 0 0 0\nv 0 1 0\nf 1 2 1\n");
    writeText("Meshes/SM_Pillar.obj", "# Pillar placeholder\nv 0 0 0\nv 0 2 0\nf 1 2 1\n");

    writeText("Blueprints/BP_Player.bp", R"({"type":"Blueprint","class":"Character"})");
    writeText("Blueprints/BP_GameMode.bp", R"({"type":"Blueprint","class":"GameMode"})");

    writeText("Audio/SFX_Click.wav", "RIFF....WAVEfmt ");
    writeText("Audio/Music_MainTheme.mp3", "ID3");

    writeText("Scripts/GameMode.lua", "function BeginPlay() end\n");
    writeText("Scripts/PlayerController.cs", "public class PlayerController {}\n");

    writeText("Animations/Anim_Idle.anim", R"({"type":"Animation","duration":2.0})");
    writeText("Animations/Anim_Walk.anim", R"({"type":"Animation","duration":1.2})");

    writeText("Maps/Level_01.scene", R"({"type":"Scene","name":"Level_01"})");
    writeText("Maps/Level_02.scene", R"({"type":"Scene","name":"Level_02"})");
}

std::string ContentAssetRegistry::MakeId(const std::string& virtualPath) const {
    return virtualPath;
}

void ContentAssetRegistry::Refresh() {
    m_Assets.clear();
    m_IdIndex.clear();
    m_PathIndex.clear();
    m_FolderVersions.clear();

    const fs::path gameRoot = fs::path(m_ContentRoot) / "Game";
    if (!fs::exists(gameRoot)) {
        NotifyRegistryRefreshed();
        return;
    }

    ScanDirectory(gameRoot.string(), "/Game");

    for (auto& asset : m_Assets) {
        if (asset.isFolder) {
            uint32_t version = 0;
            for (const auto& child : m_Assets) {
                if (child.parentPath == asset.virtualPath) {
                    version += child.contentVersion + 1;
                }
            }
            m_FolderVersions[asset.virtualPath] = version;
            asset.contentVersion = version;
        } else {
            asset.contentVersion = static_cast<uint32_t>(asset.modifiedTime & 0xFFFFFFFFu);
        }
    }

    NotifyRegistryRefreshed();
}

void ContentAssetRegistry::ScanDirectory(const std::string& diskPath, const std::string& virtualPath) {
    std::error_code ec;

    AssetRecord folder{};
    folder.id = MakeId(virtualPath);
    folder.name = virtualPath == "/Game" ? "Game" : fs::path(virtualPath).filename().string();
    folder.virtualPath = virtualPath;
    folder.diskPath = diskPath;
    folder.parentPath = virtualPath == "/Game" ? "" : fs::path(virtualPath).parent_path().string();
    folder.type = AssetType::Folder;
    folder.isFolder = true;
    folder.extension = "";
    if (fs::exists(diskPath, ec)) {
        auto ftime = fs::last_write_time(diskPath, ec);
        folder.modifiedTime = static_cast<uint64_t>(ftime.time_since_epoch().count());
    }
    m_IdIndex[folder.id] = m_Assets.size();
    m_PathIndex[folder.virtualPath] = m_Assets.size();
    m_Assets.push_back(folder);

    for (const auto& entry : fs::directory_iterator(diskPath, ec)) {
        if (ec) break;

        const std::string entryName = entry.path().filename().string();
        if (!entryName.empty() && entryName[0] == '.') continue;

        const std::string childVirtual = virtualPath + "/" + entryName;
        const std::string childDisk = entry.path().string();

        if (entry.is_directory(ec)) {
            ScanDirectory(childDisk, childVirtual);
            continue;
        }

        AssetRecord asset{};
        asset.id = MakeId(childVirtual);
        asset.name = entryName;
        asset.virtualPath = childVirtual;
        asset.diskPath = childDisk;
        asset.parentPath = virtualPath;
        asset.isFolder = false;
        asset.extension = entry.path().extension().string();
        asset.type = AssetTypeResolver::FromPath(childDisk, false);
        asset.fileSize = entry.file_size(ec);
        auto ftime = fs::last_write_time(entry, ec);
        asset.modifiedTime = static_cast<uint64_t>(ftime.time_since_epoch().count());
        asset.contentVersion = static_cast<uint32_t>(asset.modifiedTime & 0xFFFFFFFFu);

        m_IdIndex[asset.id] = m_Assets.size();
        m_PathIndex[asset.virtualPath] = m_Assets.size();
        m_Assets.push_back(asset);
    }
}

void ContentAssetRegistry::Tick(float deltaTime) {
    if (!m_Initialized) return;
    m_WatchTimer += deltaTime;
    if (m_WatchTimer < m_WatchInterval) return;
    m_WatchTimer = 0.0f;

    const fs::path gameRoot = fs::path(m_ContentRoot) / "Game";
    if (!fs::exists(gameRoot)) return;

    static uint64_t lastScanSignature = 0;
    uint64_t signature = 0;
    std::error_code ec;
    for (const auto& entry : fs::recursive_directory_iterator(gameRoot, ec)) {
        signature += static_cast<uint64_t>(entry.file_size(ec));
        auto ftime = fs::last_write_time(entry, ec);
        signature ^= static_cast<uint64_t>(ftime.time_since_epoch().count());
    }

    if (signature != lastScanSignature) {
        lastScanSignature = signature;
        Refresh();
    }
}

const AssetRecord* ContentAssetRegistry::FindById(const std::string& id) const {
    auto it = m_IdIndex.find(id);
    if (it == m_IdIndex.end()) return nullptr;
    return &m_Assets[it->second];
}

const AssetRecord* ContentAssetRegistry::FindByVirtualPath(const std::string& virtualPath) const {
    auto it = m_PathIndex.find(virtualPath);
    if (it == m_PathIndex.end()) return nullptr;
    return &m_Assets[it->second];
}

std::vector<const AssetRecord*> ContentAssetRegistry::GetChildren(const std::string& folderVirtualPath) const {
    std::vector<const AssetRecord*> children;
    for (const auto& asset : m_Assets) {
        if (asset.parentPath == folderVirtualPath) {
            children.push_back(&asset);
        }
    }
    std::sort(children.begin(), children.end(), [](const AssetRecord* a, const AssetRecord* b) {
        if (a->isFolder != b->isFolder) return a->isFolder > b->isFolder;
        return a->name < b->name;
    });
    return children;
}

std::vector<const AssetRecord*> ContentAssetRegistry::GetFolderContents(
    const std::string& folderVirtualPath, bool recursive) const
{
    std::vector<const AssetRecord*> result;
    std::function<void(const std::string&)> walk = [&](const std::string& path) {
        for (const auto& asset : m_Assets) {
            if (asset.parentPath == path) {
                if (!asset.isFolder) {
                    result.push_back(&asset);
                } else if (recursive) {
                    walk(asset.virtualPath);
                }
            }
        }
    };
    walk(folderVirtualPath);
    return result;
}

void ContentAssetRegistry::ToggleFavorite(const std::string& id) {
    auto it = m_IdIndex.find(id);
    if (it == m_IdIndex.end()) return;
    m_Assets[it->second].isFavorite = !m_Assets[it->second].isFavorite;
    if (m_OnAssetChanged) m_OnAssetChanged(m_Assets[it->second]);
}

bool ContentAssetRegistry::IsFavorite(const std::string& id) const {
    auto it = m_IdIndex.find(id);
    if (it == m_IdIndex.end()) return false;
    return m_Assets[it->second].isFavorite;
}

uint32_t ContentAssetRegistry::GetFolderContentVersion(const std::string& folderVirtualPath) const {
    auto it = m_FolderVersions.find(folderVirtualPath);
    if (it == m_FolderVersions.end()) return 0;
    return it->second;
}

void ContentAssetRegistry::NotifyRegistryRefreshed() {
    if (m_OnRegistryRefreshed) m_OnRegistryRefreshed();
}

} // namespace we::editor::contentbrowser
