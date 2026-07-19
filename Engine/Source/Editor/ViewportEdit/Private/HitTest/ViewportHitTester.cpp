#include "ViewportEditInternal.h"

#include "Core/Math/GlmInterop.h"
#include "Scene/Entity.h"

#include <cmath>
#include <limits>

#if WE_HAS_GLM
#include <glm/gtc/matrix_inverse.hpp>
#endif

namespace we::editor::viewportedit {
namespace detail {
namespace {

float EntityPickRadius(const scene::Entity& entity) {
    using scene::EntityType;
    switch (entity.Type) {
    case EntityType::Plane:
    case EntityType::Landscape:
    case EntityType::GroundPlane:
        return 2.5f;
    case EntityType::DirectionalLight:
    case EntityType::PointLight:
    case EntityType::SpotLight:
    case EntityType::CameraIcon:
    case EntityType::AudioSource:
        return 0.6f;
    default:
        return 0.75f * std::max({entity.Scale.x, entity.Scale.y, entity.Scale.z, 0.1f});
    }
}

bool RaySphere(
    const we::math::Vec3& origin,
    const we::math::Vec3& dir,
    const we::math::Vec3& center,
    float radius,
    float& outT)
{
    const float ox = origin.x - center.x;
    const float oy = origin.y - center.y;
    const float oz = origin.z - center.z;
    const float b = ox * dir.x + oy * dir.y + oz * dir.z;
    const float c = ox * ox + oy * oy + oz * oz - radius * radius;
    const float disc = b * b - c;
    if (disc < 0.f) {
        return false;
    }
    const float s = std::sqrt(disc);
    float t = -b - s;
    if (t < 0.f) {
        t = -b + s;
    }
    if (t < 0.f) {
        return false;
    }
    outT = t;
    return true;
}

} // namespace

class ViewportHitTesterImpl final : public IViewportHitTester {
public:
    explicit ViewportHitTesterImpl(IViewportContext& context)
        : m_Context(context)
    {}

    [[nodiscard]] ViewportRay ScreenToRay(float x, float y, float viewportW, float viewportH) const override {
        ViewportRay ray{};
        auto* camera = m_Context.EditorCamera();
        if (!camera || viewportW <= 1.f || viewportH <= 1.f) {
            return ray;
        }

        ray.origin = camera->GetPosition();

#if WE_HAS_GLM
        const glm::mat4 view = we::math::ToGlm(camera->GetViewMatrix());
        const glm::mat4 proj = we::math::ToGlm(camera->GetProjectionMatrix());
        const glm::mat4 inv = glm::inverse(proj * view);

        const float ndcX = (2.f * x / viewportW) - 1.f;
        const float ndcY = 1.f - (2.f * y / viewportH);
        const glm::vec4 nearClip(ndcX, ndcY, -1.f, 1.f);
        const glm::vec4 farClip(ndcX, ndcY, 1.f, 1.f);
        glm::vec4 nearWorld = inv * nearClip;
        glm::vec4 farWorld = inv * farClip;
        if (std::abs(nearWorld.w) > 1e-6f) {
            nearWorld /= nearWorld.w;
        }
        if (std::abs(farWorld.w) > 1e-6f) {
            farWorld /= farWorld.w;
        }
        const glm::vec3 dir = glm::normalize(glm::vec3(farWorld) - glm::vec3(nearWorld));
        ray.origin = we::math::FromGlm(glm::vec3(nearWorld));
        ray.direction = we::math::FromGlm(dir);
#else
        ray.direction = camera->GetForward();
#endif
        return ray;
    }

    [[nodiscard]] ViewportHit Pick(float x, float y, float viewportW, float viewportH) const override {
        const auto t0 = NowMicros();
        ViewportHit best{};
        best.distance = std::numeric_limits<float>::max();

        auto* scene = m_Context.Scene();
        if (!scene) {
            ViewportEditDiagnostics::Get().OnPick(NowMicros() - t0);
            return best;
        }

        const ViewportRay ray = ScreenToRay(x, y, viewportW, viewportH);
        const float lenSq = ray.direction.x * ray.direction.x
            + ray.direction.y * ray.direction.y
            + ray.direction.z * ray.direction.z;
        if (lenSq < 1e-12f) {
            ViewportEditDiagnostics::Get().OnPick(NowMicros() - t0);
            return best;
        }

        for (const auto& entity : scene->GetEntities()) {
            if (m_Context.Selection().IsHidden(ViewportObjectId{entity.Id})) {
                continue;
            }
            float t = 0.f;
            if (!RaySphere(ray.origin, ray.direction, entity.Position, EntityPickRadius(entity), t)) {
                continue;
            }
            if (t < best.distance) {
                best.valid = true;
                best.distance = t;
                best.object = ViewportObjectId{entity.Id};
                best.worldPoint = we::math::Vec3{
                    ray.origin.x + ray.direction.x * t,
                    ray.origin.y + ray.direction.y * t,
                    ray.origin.z + ray.direction.z * t};
            }
        }

        ViewportEditDiagnostics::Get().OnPick(NowMicros() - t0);
        return best;
    }

    [[nodiscard]] std::vector<ViewportHit> PickBox(
        const ViewportRect& rect,
        float viewportW,
        float viewportH) const override
    {
        std::vector<ViewportHit> hits;
        for (const auto id : QueryBox(rect, viewportW, viewportH)) {
            ViewportHit hit;
            hit.valid = true;
            hit.object = id;
            hits.push_back(hit);
        }
        return hits;
    }

    [[nodiscard]] std::vector<ViewportObjectId> QueryBox(
        const ViewportRect& rect,
        float viewportW,
        float viewportH) const override
    {
        std::vector<ViewportObjectId> result;
        auto* scene = m_Context.Scene();
        auto* camera = m_Context.EditorCamera();
        if (!scene || !camera || viewportW <= 1.f || viewportH <= 1.f) {
            return result;
        }

#if WE_HAS_GLM
        const glm::mat4 viewProj =
            we::math::ToGlm(camera->GetProjectionMatrix()) * we::math::ToGlm(camera->GetViewMatrix());
        for (const auto& entity : scene->GetEntities()) {
            if (m_Context.Selection().IsHidden(ViewportObjectId{entity.Id})) {
                continue;
            }
            const glm::vec4 clip = viewProj * glm::vec4(we::math::ToGlm(entity.Position), 1.f);
            if (std::abs(clip.w) < 1e-6f) {
                continue;
            }
            const float ndcX = clip.x / clip.w;
            const float ndcY = clip.y / clip.w;
            const float sx = (ndcX * 0.5f + 0.5f) * viewportW;
            const float sy = (1.f - (ndcY * 0.5f + 0.5f)) * viewportH;
            if (rect.Contains(sx, sy)) {
                result.push_back(ViewportObjectId{entity.Id});
            }
        }
#else
        (void)rect;
#endif
        return result;
    }

private:
    IViewportContext& m_Context;
};

std::unique_ptr<IViewportHitTester> CreateHitTester(IViewportContext& context) {
    return std::make_unique<ViewportHitTesterImpl>(context);
}

} // namespace detail
} // namespace we::editor::viewportedit
