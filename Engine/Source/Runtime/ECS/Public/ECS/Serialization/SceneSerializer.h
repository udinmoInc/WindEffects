#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/Export.h"
#include "ECS/Registry.h"
#include "ECS/Components/CoreComponents.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::ecs {

inline constexpr std::uint32_t kSceneSerializeVersion = 1;

struct SerializedEntity {
    Uuid uuid{};
    Entity entity{};
    std::string name;
    Entity parent{};
    std::vector<std::uint8_t> componentBlob; // versioned opaque payload (JSON/bytes)
};

struct SerializedScene {
    std::uint32_t version = kSceneSerializeVersion;
    std::vector<SerializedEntity> entities;
};

class ECS_API SceneSerializer {
public:
    static SerializedScene Capture(const Registry& registry);
    static bool Apply(Registry& registry, const SerializedScene& scene);

    static bool SaveToFile(const Registry& registry, const std::string& path);
    static bool LoadFromFile(Registry& registry, const std::string& path);

    // Prefab = subset of entities + remapped parent links.
    static SerializedScene CapturePrefab(const Registry& registry, Entity root);
    static Entity InstantiatePrefab(Registry& registry, const SerializedScene& prefab, Entity parent = {});
};

} // namespace we::runtime::ecs

#pragma warning(pop)
