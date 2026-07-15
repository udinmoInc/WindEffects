#include "ECS/Serialization/SceneSerializer.h"
#include "ECS/System.h"
#include "ECS/Components/CoreComponents.h"

#include "Core/Logger.h"

#include <fstream>
#include <sstream>
#include <unordered_map>

namespace we::runtime::ecs {

namespace {

void AppendString(std::ostream& out, const std::string& s) {
    out << s.size() << ':' << s;
}

bool ReadString(std::istream& in, std::string& out) {
    std::size_t len = 0;
    char colon = 0;
    if (!(in >> len)) {
        return false;
    }
    if (!(in >> colon) || colon != ':') {
        return false;
    }
    out.resize(len);
    if (len > 0) {
        in.read(out.data(), static_cast<std::streamsize>(len));
    }
    return static_cast<bool>(in) || len == 0;
}

} // namespace

SerializedScene SceneSerializer::Capture(const Registry& registry) {
    SerializedScene scene{};
    scene.version = kSceneSerializeVersion;

    const_cast<Registry&>(registry).ForEachLiving([&](Entity e) {
        SerializedEntity se{};
        se.entity = e;
        if (const UuidComponent* uuid = registry.TryGet<UuidComponent>(e)) {
            se.uuid = uuid->id;
        }
        if (const NameComponent* name = registry.TryGet<NameComponent>(e)) {
            se.name = name->value;
        }
        if (const HierarchyComponent* h = registry.TryGet<HierarchyComponent>(e)) {
            se.parent = h->parent;
        }

        std::ostringstream blob;
        blob << "v1;";
        if (const TransformComponent* t = registry.TryGet<TransformComponent>(e)) {
#if WE_HAS_GLM
            blob << "T:" << t->localPosition.x << ',' << t->localPosition.y << ',' << t->localPosition.z
                << ',' << t->localRotation.x << ',' << t->localRotation.y << ',' << t->localRotation.z
                << ',' << t->localScale.x << ',' << t->localScale.y << ',' << t->localScale.z << ';';
#else
            blob << "T:0,0,0,0,0,0,1,1,1;";
            (void)t;
#endif
        }
        if (const VisibilityComponent* v = registry.TryGet<VisibilityComponent>(e)) {
            blob << "V:" << (v->visible ? 1 : 0) << ';';
        }
        if (const LegacyActorComponent* legacy = registry.TryGet<LegacyActorComponent>(e)) {
            blob << "L:" << legacy->entityType << ',' << (legacy->editorOnly ? 1 : 0) << ',' << legacy->mode << ';';
        }
        if (const TagComponent* tag = registry.TryGet<TagComponent>(e)) {
            blob << "G:" << tag->mask << ',';
            AppendString(blob, tag->label);
            blob << ';';
        }
        const std::string text = blob.str();
        se.componentBlob.assign(text.begin(), text.end());
        scene.entities.push_back(std::move(se));
    });

    return scene;
}

bool SceneSerializer::Apply(Registry& registry, const SerializedScene& scene) {
    if (scene.version > kSceneSerializeVersion) {
        HE_ERROR("[ECS] Unsupported scene serialize version.");
        return false;
    }

    registry.Clear();
    std::unordered_map<std::uint64_t, Entity> remap;

    // Pass 1: create entities
    for (const SerializedEntity& se : scene.entities) {
        Entity e = se.name.empty() ? registry.Create() : registry.Create(se.name);
        remap[se.entity.id] = e;
        if (!se.uuid.IsNil()) {
            registry.Replace(e, UuidComponent{ se.uuid });
        }
    }

    // Pass 2: hierarchy + component blob
    HierarchySystem hierarchy;
    for (const SerializedEntity& se : scene.entities) {
        Entity e = remap[se.entity.id];
        if (se.parent && remap.count(se.parent.id)) {
            hierarchy.SetParent(registry, e, remap[se.parent.id]);
        }

        const std::string blob(se.componentBlob.begin(), se.componentBlob.end());
        // Minimal parsers; versioned and extensible.
        std::size_t pos = 0;
        while (pos < blob.size()) {
            const std::size_t end = blob.find(';', pos);
            const std::string token = blob.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
            pos = (end == std::string::npos) ? blob.size() : end + 1;
            if (token.rfind("T:", 0) == 0) {
                TransformComponent t{};
#if WE_HAS_GLM
                std::sscanf(token.c_str() + 2, "%f,%f,%f,%f,%f,%f,%f,%f,%f",
                    &t.localPosition.x, &t.localPosition.y, &t.localPosition.z,
                    &t.localRotation.x, &t.localRotation.y, &t.localRotation.z,
                    &t.localScale.x, &t.localScale.y, &t.localScale.z);
#endif
                t.dirty = true;
                registry.Replace(e, t);
            } else if (token.rfind("V:", 0) == 0) {
                VisibilityComponent v{};
                int visible = 1;
                std::sscanf(token.c_str() + 2, "%d", &visible);
                v.visible = visible != 0;
                registry.Replace(e, v);
            } else if (token.rfind("L:", 0) == 0) {
                LegacyActorComponent legacy{};
                int editorOnly = 0;
                std::sscanf(token.c_str() + 2, "%d,%d,%d", &legacy.entityType, &editorOnly, &legacy.mode);
                legacy.editorOnly = editorOnly != 0;
                registry.Replace(e, legacy);
            } else if (token.rfind("G:", 0) == 0) {
                TagComponent tag{};
                unsigned long long mask = 0;
                std::sscanf(token.c_str() + 2, "%llu", &mask);
                tag.mask = static_cast<std::uint64_t>(mask);
                registry.Replace(e, tag);
            }
        }
    }
    return true;
}

bool SceneSerializer::SaveToFile(const Registry& registry, const std::string& path) {
    const SerializedScene scene = Capture(registry);
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }
    out << "WEECS " << scene.version << "\n";
    out << scene.entities.size() << "\n";
    for (const SerializedEntity& se : scene.entities) {
        out << se.uuid.ToString() << "\n";
        out << se.entity.id << " " << se.parent.id << "\n";
        AppendString(out, se.name);
        out << "\n";
        out << se.componentBlob.size() << "\n";
        if (!se.componentBlob.empty()) {
            out.write(reinterpret_cast<const char*>(se.componentBlob.data()),
                static_cast<std::streamsize>(se.componentBlob.size()));
        }
        out << "\n";
    }
    return static_cast<bool>(out);
}

bool SceneSerializer::LoadFromFile(Registry& registry, const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    std::string magic;
    std::uint32_t version = 0;
    in >> magic >> version;
    if (magic != "WEECS") {
        return false;
    }
    std::size_t count = 0;
    in >> count;
    SerializedScene scene{};
    scene.version = version;
    scene.entities.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        SerializedEntity se{};
        std::string uuidStr;
        in >> uuidStr;
        se.uuid = Uuid::FromString(uuidStr);
        in >> se.entity.id >> se.parent.id;
        ReadString(in, se.name);
        std::size_t blobSize = 0;
        in >> blobSize;
        in.get(); // newline
        se.componentBlob.resize(blobSize);
        if (blobSize > 0) {
            in.read(reinterpret_cast<char*>(se.componentBlob.data()),
                static_cast<std::streamsize>(blobSize));
        }
        scene.entities.push_back(std::move(se));
    }
    return Apply(registry, scene);
}

SerializedScene SceneSerializer::CapturePrefab(const Registry& registry, Entity root) {
    SerializedScene prefab = Capture(registry);
    // Keep only subtree of root (simple filter by walking parent links).
    SerializedScene out{};
    out.version = prefab.version;
    for (const SerializedEntity& se : prefab.entities) {
        Entity walk = se.entity;
        bool keep = false;
        while (walk) {
            if (walk == root) {
                keep = true;
                break;
            }
            const HierarchyComponent* h = registry.TryGet<HierarchyComponent>(walk);
            walk = h ? h->parent : Entity{};
        }
        if (keep) {
            out.entities.push_back(se);
        }
    }
    return out;
}

Entity SceneSerializer::InstantiatePrefab(Registry& registry, const SerializedScene& prefab, Entity parent) {
    // Remap into a fresh registry slice by Apply into temporary then... 
    // Simpler: capture new entities from prefab blob with remapped IDs.
    Registry temp;
    if (!Apply(temp, prefab)) {
        return {};
    }
    const SerializedScene captured = Capture(temp);
    std::unordered_map<std::uint64_t, Entity> remap;
    Entity rootNew{};
    for (const SerializedEntity& se : captured.entities) {
        Entity e = se.name.empty() ? registry.Create() : registry.Create(se.name);
        remap[se.entity.id] = e;
        if (!rootNew) {
            rootNew = e;
        }
    }
    HierarchySystem hierarchy;
    for (const SerializedEntity& se : captured.entities) {
        Entity e = remap[se.entity.id];
        if (se.parent && remap.count(se.parent.id)) {
            hierarchy.SetParent(registry, e, remap[se.parent.id]);
        } else if (parent) {
            hierarchy.SetParent(registry, e, parent);
        }
    }
    return rootNew;
}

} // namespace we::runtime::ecs
