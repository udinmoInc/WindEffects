#include "ViewportEditInternal.h"
#include "Scene/Entity.h"

#include <algorithm>

namespace we::editor::viewportedit {
namespace detail {

class ViewportCameraControllerImpl final : public IViewportCameraController {
public:
    explicit ViewportCameraControllerImpl(IViewportContext& context)
        : m_Context(context)
    {}

    void SetProjection(CameraProjection projection) override { m_Projection = projection; }
    [[nodiscard]] CameraProjection GetProjection() const noexcept override { return m_Projection; }

    void FocusSelected(std::span<const ViewportObjectId> ids) override { FrameSelection(ids); }

    void FrameSelection(std::span<const ViewportObjectId> ids) override {
        auto* camera = m_Context.EditorCamera();
        auto* scene = m_Context.Scene();
        if (!camera || !scene || ids.empty()) {
            return;
        }
        we::math::Vec3 sum{};
        int count = 0;
        for (const auto id : ids) {
            if (const auto* entity = scene->FindEntityById(id.value)) {
                sum.x += entity->Position.x;
                sum.y += entity->Position.y;
                sum.z += entity->Position.z;
                ++count;
            }
        }
        if (count > 0) {
            const we::math::Vec3 center{sum.x / count, sum.y / count, sum.z / count};
            float distance = 8.0f;
            for (const auto id : ids) {
                if (const auto* entity = scene->FindEntityById(id.value)) {
                    if (entity->Type == we::runtime::scene::EntityType::Landscape) {
                        distance = std::max(distance, 600.0f);
                    }
                }
            }
            camera->Focus(center, distance);
        }
    }

    void Orbit(float dx, float dy) override {
        if (auto* camera = m_Context.EditorCamera()) {
            camera->ProcessMouseOrbit(dx, dy);
        }
    }

    void Pan(float dx, float dy) override {
        if (auto* camera = m_Context.EditorCamera()) {
            camera->ProcessMousePan(dx, dy);
        }
    }

    void Zoom(float delta) override {
        if (auto* camera = m_Context.EditorCamera()) {
            camera->ProcessMouseScroll(delta);
        }
    }

    void FlyLook(float dx, float dy) override {
        if (auto* camera = m_Context.EditorCamera()) {
            camera->ProcessFlyLook(dx, dy);
        }
    }

    [[nodiscard]] we::math::Vec3 GetPosition() const override {
        if (auto* camera = m_Context.EditorCamera()) {
            return camera->GetPosition();
        }
        return {};
    }

    [[nodiscard]] we::math::Mat4 GetViewMatrix() const override {
        if (auto* camera = m_Context.EditorCamera()) {
            return camera->GetViewMatrix();
        }
        return {};
    }

    [[nodiscard]] we::math::Mat4 GetProjectionMatrix() const override {
        if (auto* camera = m_Context.EditorCamera()) {
            return camera->GetProjectionMatrix();
        }
        return {};
    }

    CameraBookmark SaveBookmark(std::string_view name) override {
        CameraBookmark bm;
        bm.name = std::string(name);
        if (auto* camera = m_Context.EditorCamera()) {
            bm.position = camera->GetPosition();
            bm.lookAt = camera->GetLookAt();
            bm.fovDegrees = camera->GetFov();
        }
        bm.projection = m_Projection;
        m_Bookmarks[bm.name] = bm;
        return bm;
    }

    bool RestoreBookmark(std::string_view name) override {
        auto it = m_Bookmarks.find(std::string(name));
        if (it == m_Bookmarks.end()) {
            return false;
        }
        auto* camera = m_Context.EditorCamera();
        if (!camera) {
            return false;
        }
        camera->Focus(it->second.lookAt);
        camera->SetOrbitPivot(it->second.lookAt);
        m_Projection = it->second.projection;
        return true;
    }

    [[nodiscard]] std::vector<CameraBookmark> ListBookmarks() const override {
        std::vector<CameraBookmark> list;
        list.reserve(m_Bookmarks.size());
        for (const auto& [_, bm] : m_Bookmarks) {
            list.push_back(bm);
        }
        return list;
    }

private:
    IViewportContext& m_Context;
    CameraProjection m_Projection = CameraProjection::Perspective;
    std::unordered_map<std::string, CameraBookmark> m_Bookmarks;
};

class ViewportSnapProviderImpl final : public IViewportSnapProvider {
public:
    explicit ViewportSnapProviderImpl(SnapSettings settings)
        : m_Settings(std::move(settings))
    {}

    void SetSettings(const SnapSettings& settings) override { m_Settings = settings; }
    [[nodiscard]] const SnapSettings& GetSettings() const noexcept override { return m_Settings; }

    [[nodiscard]] we::math::Vec3 SnapTranslation(const we::math::Vec3& value) const override {
        if (!m_Settings.gridEnabled || m_Settings.gridSize <= 0.f) {
            return value;
        }
        const float s = m_Settings.gridSize;
        auto snap = [s](float v) { return std::round(v / s) * s; };
        return {snap(value.x), snap(value.y), snap(value.z)};
    }

    [[nodiscard]] we::math::Vec3 SnapRotationDegrees(const we::math::Vec3& eulerDegrees) const override {
        if (!m_Settings.rotationEnabled || m_Settings.rotationDegrees <= 0.f) {
            return eulerDegrees;
        }
        const float s = m_Settings.rotationDegrees;
        auto snap = [s](float v) { return std::round(v / s) * s; };
        return {snap(eulerDegrees.x), snap(eulerDegrees.y), snap(eulerDegrees.z)};
    }

    [[nodiscard]] we::math::Vec3 SnapScale(const we::math::Vec3& scale) const override {
        if (!m_Settings.scaleEnabled || m_Settings.scaleStep <= 0.f) {
            return scale;
        }
        const float s = m_Settings.scaleStep;
        auto snap = [s](float v) { return std::round(v / s) * s; };
        return {snap(scale.x), snap(scale.y), snap(scale.z)};
    }

private:
    SnapSettings m_Settings;
};

class ViewportGridProviderImpl final : public IViewportGridProvider {
public:
    explicit ViewportGridProviderImpl(float cellSize)
        : m_CellSize(cellSize)
    {}

    void SetVisible(bool visible) override { m_Visible = visible; }
    [[nodiscard]] bool IsVisible() const noexcept override { return m_Visible; }
    void SetCellSize(float size) override { m_CellSize = std::max(0.001f, size); }
    [[nodiscard]] float GetCellSize() const noexcept override { return m_CellSize; }

private:
    bool m_Visible = true;
    float m_CellSize = 1.f;
};

std::unique_ptr<IViewportCameraController> CreateCameraController(IViewportContext& context) {
    return std::make_unique<ViewportCameraControllerImpl>(context);
}

std::unique_ptr<IViewportSnapProvider> CreateSnapProvider(SnapSettings settings) {
    return std::make_unique<ViewportSnapProviderImpl>(std::move(settings));
}

std::unique_ptr<IViewportGridProvider> CreateGridProvider(float cellSize) {
    return std::make_unique<ViewportGridProviderImpl>(cellSize);
}

} // namespace detail
} // namespace we::editor::viewportedit
