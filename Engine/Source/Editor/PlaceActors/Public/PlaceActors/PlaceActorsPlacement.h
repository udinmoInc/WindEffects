#pragma once

#include "PlaceActors/Export.h"

#include "Core/Math/Types.h"
#include <memory>
#include <string>

namespace we::runtime::engine { class EditorCamera; }
namespace we::runtime::scene { class Scene; }

namespace we::programs::editor {

class PLACEACTORS_API PlaceActorsPlacement {
public:
    static PlaceActorsPlacement& Get();

    void BindScene(const std::shared_ptr<we::runtime::scene::Scene>& scene,
                   const std::shared_ptr<we::runtime::engine::EditorCamera>& camera);

    bool SpawnTool(const std::string& toolId);
    bool SpawnToolAt(const std::string& toolId, const we::math::Vec3& worldPosition);

    void BeginDragPlacement(const std::string& toolId);
    void CancelDragPlacement();
    bool HasActivePlacement() const { return !m_ActivePlacementToolId.empty(); }
    const std::string& GetActivePlacementToolId() const { return m_ActivePlacementToolId; }
    bool TryPlaceAtViewportPoint(float viewportLocalX, float viewportLocalY);

private:
    PlaceActorsPlacement() = default;

    we::math::Vec3 ComputeSpawnPosition() const;

    std::weak_ptr<we::runtime::scene::Scene> m_Scene;
    std::weak_ptr<we::runtime::engine::EditorCamera> m_Camera;
    std::string m_ActivePlacementToolId;
};

} // namespace we::programs::editor
