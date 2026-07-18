#include "ECS/System.h"
#include "ECS/Components/CoreComponents.h"
#include "ECS/World.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_set>

#include "Core/Math/GlmInterop.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace we::runtime::ecs {

namespace {

we::math::Mat4 ComposeLocal(const TransformComponent& t) {
    const glm::mat4 T = glm::translate(glm::mat4(1.0f), we::math::ToGlm(t.localPosition));
    const glm::mat4 Rx = glm::rotate(glm::mat4(1.0f), glm::radians(t.localRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::mat4 Ry = glm::rotate(glm::mat4(1.0f), glm::radians(t.localRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 Rz = glm::rotate(glm::mat4(1.0f), glm::radians(t.localRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    const glm::mat4 S = glm::scale(glm::mat4(1.0f), we::math::ToGlm(t.localScale));
    return we::math::FromGlm(T * Ry * Rx * Rz * S);
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
    UpdateParallel(registry, deltaSeconds);
}

void SystemScheduler::UpdateParallel(Registry& registry, float deltaSeconds) {
    World& world = registry.GetWorld();
    std::vector<std::size_t> batch;
    batch.reserve(m_Systems.size());

    for (std::size_t i = 0; i < m_Systems.size(); ++i) {
        if (batch.empty()) {
            batch.push_back(i);
            continue;
        }
        bool compatible = true;
        for (std::size_t j : batch) {
            if (!AreCompatible(*m_Systems[j], *m_Systems[i])) {
                compatible = false;
                break;
            }
        }
        if (!compatible) {
            for (std::size_t j : batch) {
                m_Systems[j]->Update(registry, deltaSeconds);
            }
            batch.clear();
        }
        batch.push_back(i);
    }
    for (std::size_t j : batch) {
        m_Systems[j]->Update(registry, deltaSeconds);
    }

    world.FlushCommands();
    (void)world.Jobs().WorkerCount();
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

    // Reset depth for detached subtree.
    std::queue<Entity> q;
    h->depth = 0;
    q.push(child);
    while (!q.empty()) {
        Entity e = q.front();
        q.pop();
        HierarchyComponent* eh = registry.TryGet<HierarchyComponent>(e);
        if (!eh) {
            continue;
        }
        Entity c = eh->firstChild;
        while (c) {
            if (HierarchyComponent* ch = registry.TryGet<HierarchyComponent>(c)) {
                ch->depth = static_cast<std::uint16_t>(eh->depth + 1);
                q.push(c);
                c = ch->nextSibling;
            } else {
                break;
            }
        }
    }
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

    // Recompute depth for the whole subtree (required for parallel transform by depth).
    std::queue<Entity> q;
    h->depth = static_cast<std::uint16_t>(p->depth + 1);
    q.push(child);
    while (!q.empty()) {
        Entity e = q.front();
        q.pop();
        HierarchyComponent* eh = registry.TryGet<HierarchyComponent>(e);
        if (!eh) {
            continue;
        }
        Entity c = eh->firstChild;
        while (c) {
            if (HierarchyComponent* ch = registry.TryGet<HierarchyComponent>(c)) {
                ch->depth = static_cast<std::uint16_t>(eh->depth + 1);
                q.push(c);
                c = ch->nextSibling;
            } else {
                break;
            }
        }
    }
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
    if (parent) {
        if (TransformComponent* pt = registry.TryGet<TransformComponent>(parent)) {
            pt->dirty = true;
        }
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
    World& world = registry.GetWorld();

    // Bucket entities by hierarchy depth so parents finish before children.
    // Within each depth level, world-matrix updates are independent and parallel.
    std::vector<std::vector<Entity>> byDepth;
    byDepth.resize(1);

    registry.ViewAll<TransformComponent, HierarchyComponent>().Each(
        [&](Entity e, TransformComponent&, HierarchyComponent& h) {
            const std::size_t depth = static_cast<std::size_t>(h.depth);
            if (depth >= byDepth.size()) {
                byDepth.resize(depth + 1u);
            }
            byDepth[depth].push_back(e);
        });

    for (std::vector<Entity>& level : byDepth) {
        if (level.empty()) {
            continue;
        }

        auto updateOne = [&](std::size_t index) {
            Entity e = level[index];
            TransformComponent* t = registry.TryGet<TransformComponent>(e);
            HierarchyComponent* h = registry.TryGet<HierarchyComponent>(e);
            if (!t || !h || !t->dirty) {
                return;
            }

            const we::math::Mat4 local = ComposeLocal(*t);
            if (h->parent) {
                if (const TransformComponent* pt = registry.TryGet<TransformComponent>(h->parent)) {
                    t->worldMatrix = pt->worldMatrix * local;
                } else {
                    t->worldMatrix = local;
                }
            } else {
                t->worldMatrix = local;
            }
            t->dirty = false;

            // Mark children dirty so the next depth level recomposes.
            Entity child = h->firstChild;
            while (child) {
                if (TransformComponent* ct = registry.TryGet<TransformComponent>(child)) {
                    ct->dirty = true;
                }
                HierarchyComponent* ch = registry.TryGet<HierarchyComponent>(child);
                child = ch ? ch->nextSibling : Entity{};
            }
        };

        if (level.size() > 64) {
            world.Jobs().ParallelFor(level.size(), updateOne);
        } else {
            for (std::size_t i = 0; i < level.size(); ++i) {
                updateOne(i);
            }
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
    (void)registry.ViewAll<DirectionalLightComponent, TransformComponent>().Count();
    (void)registry.ViewAll<PointLightComponent, TransformComponent>().Count();
    (void)registry.ViewAll<SpotLightComponent, TransformComponent>().Count();
}

const std::vector<ComponentAccess>& RenderExtractionSystem::Access() const {
    static const std::vector<ComponentAccess> access = {
        { ComponentTypeRegistry::Get().Id<TransformComponent>(), AccessMode::Read },
        { ComponentTypeRegistry::Get().Id<StaticMeshComponent>(), AccessMode::Read },
        { ComponentTypeRegistry::Get().Id<MaterialComponent>(), AccessMode::Read },
        { ComponentTypeRegistry::Get().Id<VisibilityComponent>(), AccessMode::Read },
        { ComponentTypeRegistry::Get().Id<DirectionalLightComponent>(), AccessMode::Read },
        { ComponentTypeRegistry::Get().Id<PointLightComponent>(), AccessMode::Read },
        { ComponentTypeRegistry::Get().Id<SpotLightComponent>(), AccessMode::Read },
        { ComponentTypeRegistry::Get().Id<CameraComponent>(), AccessMode::Read },
        { ComponentTypeRegistry::Get().Id<SkyAtmosphereComponent>(), AccessMode::Read },
        { ComponentTypeRegistry::Get().Id<VolumetricCloudComponent>(), AccessMode::Read },
        { ComponentTypeRegistry::Get().Id<TerrainComponent>(), AccessMode::Read },
        { ComponentTypeRegistry::Get().Id<WaterComponent>(), AccessMode::Read },
    };
    return access;
}

void RenderExtractionSystem::Update(Registry& registry, float /*deltaSeconds*/) {
    ++m_FrameIndex;
    ExtractRenderFrame(registry.GetWorld(), m_Frame);
    m_Frame.frameIndex = m_FrameIndex;
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
    scheduler.Add(std::make_unique<RenderExtractionSystem>());
}

} // namespace we::runtime::ecs
