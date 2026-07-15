#include "ECS/System.h"
#include "ECS/Components/CoreComponents.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_set>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace we::runtime::ecs {

namespace {

glm::mat4 ComposeLocal(const TransformComponent& t) {
    const glm::mat4 T = glm::translate(glm::mat4(1.0f), t.localPosition);
    const glm::quat q = glm::quat(glm::radians(t.localRotation));
    const glm::mat4 R = glm::mat4_cast(q);
    const glm::mat4 S = glm::scale(glm::mat4(1.0f), t.localScale);
    return T * R * S;
}

} // namespace

void SystemScheduler::Add(std::unique_ptr<ISystem> system) {
    m_Systems.push_back(std::move(system));
}

void SystemScheduler::Clear() {
    m_Systems.clear();
}

void SystemScheduler::OnCreate(Registry& registry) {
    for (auto& system : m_Systems) {
        system->OnCreate(registry);
    }
}

void SystemScheduler::OnDestroy(Registry& registry) {
    for (auto& system : m_Systems) {
        system->OnDestroy(registry);
    }
}

void SystemScheduler::Update(Registry& registry, float deltaSeconds) {
    // Sequential execution; AreCompatible() enables future parallel batches.
    for (auto& system : m_Systems) {
        system->Update(registry, deltaSeconds);
    }
}

bool SystemScheduler::AreCompatible(const ISystem& a, const ISystem& b) {
    const auto& aa = a.Access();
    const auto& bb = b.Access();
    for (const ComponentAccess& left : aa) {
        for (const ComponentAccess& right : bb) {
            if (left.type != right.type) {
                continue;
            }
            if (left.mode == AccessMode::Write || right.mode == AccessMode::Write) {
                return false;
            }
        }
    }
    return true;
}

void HierarchySystem::RelinkRemove(Registry& registry, Entity child) {
    HierarchyComponent* h = registry.TryGet<HierarchyComponent>(child);
    if (!h) {
        return;
    }
    if (h->prevSibling) {
        if (HierarchyComponent* prev = registry.TryGet<HierarchyComponent>(h->prevSibling)) {
            prev->nextSibling = h->nextSibling;
        }
    }
    if (h->nextSibling) {
        if (HierarchyComponent* next = registry.TryGet<HierarchyComponent>(h->nextSibling)) {
            next->prevSibling = h->prevSibling;
        }
    }
    if (h->parent) {
        if (HierarchyComponent* parent = registry.TryGet<HierarchyComponent>(h->parent)) {
            if (parent->firstChild == child) {
                parent->firstChild = h->nextSibling;
            }
        }
    }
    h->parent = {};
    h->nextSibling = {};
    h->prevSibling = {};
    h->depth = 0;
}

void HierarchySystem::RelinkInsert(Registry& registry, Entity child, Entity parent) {
    HierarchyComponent* h = registry.TryGet<HierarchyComponent>(child);
    HierarchyComponent* p = registry.TryGet<HierarchyComponent>(parent);
    if (!h || !p) {
        return;
    }
    h->parent = parent;
    h->prevSibling = {};
    h->nextSibling = p->firstChild;
    if (p->firstChild) {
        if (HierarchyComponent* oldFirst = registry.TryGet<HierarchyComponent>(p->firstChild)) {
            oldFirst->prevSibling = child;
        }
    }
    p->firstChild = child;
    h->depth = static_cast<std::uint16_t>(p->depth + 1);
}

void HierarchySystem::Detach(Registry& registry, Entity child) {
    RelinkRemove(registry, child);
    registry.NotifyChanged();
}

void HierarchySystem::SetParent(Registry& registry, Entity child, Entity parent) {
    if (!registry.Valid(child)) {
        return;
    }
    // Prevent cycles with iterative walk (no recursion).
    if (parent) {
        Entity walk = parent;
        while (walk) {
            if (walk == child) {
                return;
            }
            HierarchyComponent* wh = registry.TryGet<HierarchyComponent>(walk);
            if (!wh) {
                break;
            }
            walk = wh->parent;
        }
    }
    RelinkRemove(registry, child);
    if (parent) {
        RelinkInsert(registry, child, parent);
    }
    if (TransformComponent* t = registry.TryGet<TransformComponent>(child)) {
        t->dirty = true;
    }
    registry.NotifyChanged();
}

void HierarchySystem::Update(Registry& /*registry*/, float /*deltaSeconds*/) {
    // Parenting is imperative via SetParent; structural integrity checked there.
}

const std::vector<ComponentAccess>& TransformSystem::Access() const {
    static const std::vector<ComponentAccess> access = {
        { ComponentTypeRegistry::Get().Id<TransformComponent>(), AccessMode::Write },
        { ComponentTypeRegistry::Get().Id<HierarchyComponent>(), AccessMode::Read },
    };
    return access;
}

void TransformSystem::MarkDirty(Registry& registry, Entity entity) {
    TransformComponent* t = registry.TryGet<TransformComponent>(entity);
    if (!t) {
        return;
    }
    t->dirty = true;
    // Iterative dirty propagation down the tree.
    std::queue<Entity> q;
    if (HierarchyComponent* h = registry.TryGet<HierarchyComponent>(entity)) {
        Entity child = h->firstChild;
        while (child) {
            q.push(child);
            HierarchyComponent* ch = registry.TryGet<HierarchyComponent>(child);
            child = ch ? ch->nextSibling : Entity{};
        }
    }
    while (!q.empty()) {
        Entity e = q.front();
        q.pop();
        if (TransformComponent* ct = registry.TryGet<TransformComponent>(e)) {
            ct->dirty = true;
        }
        HierarchyComponent* h = registry.TryGet<HierarchyComponent>(e);
        if (!h) {
            continue;
        }
        Entity child = h->firstChild;
        while (child) {
            q.push(child);
            HierarchyComponent* ch = registry.TryGet<HierarchyComponent>(child);
            child = ch ? ch->nextSibling : Entity{};
        }
    }
}

void TransformSystem::Update(Registry& registry, float /*deltaSeconds*/) {
    // Build root list then BFS for stable non-recursive world updates.
    std::vector<Entity> roots;
    registry.ViewAll<TransformComponent, HierarchyComponent>().Each(
        [&](Entity e, TransformComponent&, HierarchyComponent& h) {
            if (!h.parent) {
                roots.push_back(e);
            }
        });

    std::queue<Entity> q;
    for (Entity root : roots) {
        q.push(root);
    }

    while (!q.empty()) {
        Entity e = q.front();
        q.pop();
        TransformComponent* t = registry.TryGet<TransformComponent>(e);
        HierarchyComponent* h = registry.TryGet<HierarchyComponent>(e);
        if (!t || !h) {
            continue;
        }

        if (t->dirty) {
            const glm::mat4 local = ComposeLocal(*t);
            if (h->parent) {
                if (TransformComponent* pt = registry.TryGet<TransformComponent>(h->parent)) {
                    t->worldMatrix = pt->worldMatrix * local;
                } else {
                    t->worldMatrix = local;
                }
            } else {
                t->worldMatrix = local;
            }
            t->dirty = false;

            Entity child = h->firstChild;
            while (child) {
                if (TransformComponent* ct = registry.TryGet<TransformComponent>(child)) {
                    ct->dirty = true;
                }
                HierarchyComponent* ch = registry.TryGet<HierarchyComponent>(child);
                child = ch ? ch->nextSibling : Entity{};
            }
        }

        Entity child = h->firstChild;
        while (child) {
            q.push(child);
            HierarchyComponent* ch = registry.TryGet<HierarchyComponent>(child);
            child = ch ? ch->nextSibling : Entity{};
        }
    }
}

void VisibilitySystem::Update(Registry& registry, float /*deltaSeconds*/) {
    // Placeholder for frustum / occlusion gates — reads VisibilityComponent.
    (void)registry;
}

void CameraSystem::Update(Registry& registry, float /*deltaSeconds*/) {
    m_Primary = {};
    registry.ViewAll<CameraComponent, TransformComponent>().Each(
        [&](Entity e, CameraComponent& cam, TransformComponent&) {
            if (cam.primary) {
                m_Primary = e;
            }
        });
}

void LightingSystem::Update(Registry& registry, float /*deltaSeconds*/) {
    // Collect lights for renderer consumption in a later pass.
    (void)registry.ViewAll<DirectionalLightComponent, TransformComponent>().Count();
    (void)registry.ViewAll<PointLightComponent, TransformComponent>().Count();
    (void)registry.ViewAll<SpotLightComponent, TransformComponent>().Count();
}

void RenderSystem::Update(Registry& registry, float /*deltaSeconds*/) {
    m_LastDrawCount = registry.ViewAll<TransformComponent, StaticMeshComponent, MaterialComponent>().Count();
}

void SkyAtmosphereSystem::Update(Registry& registry, float /*deltaSeconds*/) {
    (void)registry.ViewAll<SkyAtmosphereComponent, TransformComponent>().Count();
}

void VolumetricCloudSystem::Update(Registry& registry, float /*deltaSeconds*/) {
    (void)registry.ViewAll<VolumetricCloudComponent, TransformComponent>().Count();
}

void TerrainEcsSystem::Update(Registry& registry, float /*deltaSeconds*/) {
    (void)registry.ViewAll<TerrainComponent, TransformComponent>().Count();
}

void PhysicsSystem::Update(Registry& registry, float /*deltaSeconds*/) {
    (void)registry.ViewAll<RigidBodyComponent, TransformComponent, ColliderComponent>().Count();
}

void AnimationSystem::Update(Registry& registry, float deltaSeconds) {
    registry.ViewAll<AnimationComponent>().Each([&](Entity, AnimationComponent& anim) {
        if (!anim.playing) {
            return;
        }
        anim.time += deltaSeconds * anim.speed;
    });
}

void AudioSystem::Update(Registry& registry, float /*deltaSeconds*/) {
    (void)registry.ViewAll<AudioSourceComponent, TransformComponent>().Count();
}

void RegisterDefaultSystems(SystemScheduler& scheduler) {
    scheduler.Add(std::make_unique<HierarchySystem>());
    scheduler.Add(std::make_unique<TransformSystem>());
    scheduler.Add(std::make_unique<VisibilitySystem>());
    scheduler.Add(std::make_unique<CameraSystem>());
    scheduler.Add(std::make_unique<LightingSystem>());
    scheduler.Add(std::make_unique<SkyAtmosphereSystem>());
    scheduler.Add(std::make_unique<VolumetricCloudSystem>());
    scheduler.Add(std::make_unique<TerrainEcsSystem>());
    scheduler.Add(std::make_unique<PhysicsSystem>());
    scheduler.Add(std::make_unique<AnimationSystem>());
    scheduler.Add(std::make_unique<AudioSystem>());
    scheduler.Add(std::make_unique<RenderSystem>());
}

} // namespace we::runtime::ecs
