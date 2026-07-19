#include "WorldInternal.h"

#include "Scene/Entity.h"

#include <algorithm>
#include <cmath>
#include <queue>

namespace we::runtime::world::detail {

void ActorImpl::BeginPlay() {
    if (lifecycle == ActorLifecycle::Destroyed || lifecycle == ActorLifecycle::Playing) {
        return;
    }
    lifecycle = ActorLifecycle::Playing;
    pendingBeginPlay = false;
    WorldDiagnostics::Get().OnBeginPlay();
}

void ActorImpl::EndPlay(EndPlayReason reason) {
    if (lifecycle == ActorLifecycle::Destroyed) {
        return;
    }
    endPlayReason = reason;
    lifecycle = ActorLifecycle::Destroyed;
    pendingEndPlay = false;
    WorldDiagnostics::Get().OnEndPlay();
}

ActorRegistryImpl::ActorRegistryImpl(WorldImpl& world) : m_World(world) {}

ActorHandle ActorRegistryImpl::Spawn(const ActorSpawnParams& params, LevelHandle level) {
    std::unique_lock lock(m_Mutex);

    LevelHandle targetLevel = level.IsValid() ? level : m_World.m_PersistentLevel;
    LevelImpl* levelImpl = m_World.GetLevel(targetLevel);
    if (!levelImpl || !levelImpl->scene) {
        return {};
    }

    scene::Scene& scene = *levelImpl->scene;
    const std::string actorName = params.name[0] != '\0' ? std::string(params.name) : std::string("Actor");
    scene.CreateEntity(actorName, scene::EntityType::EmptyActor);
    if (scene.GetEntities().empty()) {
        return {};
    }
    const std::uint64_t entityId = scene.GetEntities().back().Id;

    std::uint32_t index = 0;
    for (; index < m_Slots.size(); ++index) {
        if (!m_Slots[index]) {
            break;
        }
    }
    if (index == m_Slots.size()) {
        m_Slots.emplace_back();
    }

    auto actor = std::make_unique<ActorImpl>();
    actor->handle.index = index;
    actor->handle.generation = m_NextGeneration++;
    if (actor->handle.generation == 0) {
        actor->handle.generation = m_NextGeneration++;
    }

    actor->guid = params.guid.IsNil() ? WorldGuid::Generate() : params.guid;
    CopyName(actor->name, actorName);
    actor->lifecycle = ActorLifecycle::Constructed;
    actor->world = m_World.m_Handle;
    actor->level = targetLevel;
    actor->layer = params.layer;
    actor->tags = params.tags;
    actor->entityId = entityId;
    actor->parent = params.parent;
    actor->localPosition = {params.localPosition[0], params.localPosition[1], params.localPosition[2]};
    actor->localRotation = {params.localRotation[0], params.localRotation[1], params.localRotation[2]};
    actor->localScale = {params.localScale[0], params.localScale[1], params.localScale[2]};
    actor->pendingBeginPlay = params.beginPlayImmediately;

    // Sync transform into ECS.
    ecs::Entity entity{entityId};
    if (auto* transform = scene.World().TryGet<ecs::TransformComponent>(entity)) {
        transform->localPosition = {actor->localPosition.x, actor->localPosition.y, actor->localPosition.z};
        transform->localRotation = {actor->localRotation.x, actor->localRotation.y, actor->localRotation.z};
        transform->localScale = {actor->localScale.x, actor->localScale.y, actor->localScale.z};
        transform->dirty = true;
    }
    if (auto* uuid = scene.World().TryGet<ecs::UuidComponent>(entity)) {
        std::memcpy(uuid->id.bytes.data(), actor->guid.bytes.data(), 16);
    }

    ActorHandle handle = actor->handle;
    m_Slots[index] = std::move(actor);
    m_All.push_back(handle);
    m_ByGuid[m_Slots[index]->guid] = handle;
    m_ByEntity[entityId] = handle;
    levelImpl->actors.push_back(handle);

    if (params.parent.IsValid()) {
        lock.unlock();
        m_World.m_Hierarchy.SetParent(handle, params.parent);
    }

    WorldDiagnostics::Get().OnActorSpawned();

    if (m_World.m_RuntimeDeps.physicsHook) {
        if (auto* a = TryGet(handle)) {
            m_World.m_RuntimeDeps.physicsHook->OnActorSpawned(m_World, *a);
        }
    }
    if (m_World.m_RuntimeDeps.networkHook) {
        if (auto* a = TryGet(handle)) {
            m_World.m_RuntimeDeps.networkHook->OnActorSpawned(m_World, *a);
        }
    }

    return handle;
}

bool ActorRegistryImpl::Destroy(ActorHandle actor, EndPlayReason reason) {
    std::unique_lock lock(m_Mutex);
    if (!actor.IsValid() || actor.index >= m_Slots.size() || !m_Slots[actor.index]) {
        return false;
    }
    ActorImpl* impl = m_Slots[actor.index].get();
    if (impl->handle.generation != actor.generation) {
        return false;
    }

    if (m_World.m_RuntimeDeps.physicsHook) {
        m_World.m_RuntimeDeps.physicsHook->OnActorDestroyed(m_World, *impl);
    }
    if (m_World.m_RuntimeDeps.networkHook) {
        m_World.m_RuntimeDeps.networkHook->OnActorDestroyed(m_World, *impl);
    }

    impl->EndPlay(reason);

    LevelImpl* levelImpl = m_World.GetLevel(impl->level);
    if (levelImpl && levelImpl->scene) {
        const int idx = levelImpl->scene->FindEntityIndexById(impl->entityId);
        if (idx >= 0) {
            levelImpl->scene->DestroyEntity(static_cast<std::size_t>(idx));
        }
        auto& actors = levelImpl->actors;
        actors.erase(std::remove(actors.begin(), actors.end(), actor), actors.end());
    }

    m_ByGuid.erase(impl->guid);
    m_ByEntity.erase(impl->entityId);
    m_All.erase(std::remove(m_All.begin(), m_All.end(), actor), m_All.end());
    m_Slots[actor.index].reset();

    WorldDiagnostics::Get().OnActorDestroyed();
    return true;
}

IActor* ActorRegistryImpl::TryGet(ActorHandle handle) noexcept {
    if (!handle.IsValid() || handle.index >= m_Slots.size() || !m_Slots[handle.index]) {
        return nullptr;
    }
    if (m_Slots[handle.index]->handle.generation != handle.generation) {
        return nullptr;
    }
    return m_Slots[handle.index].get();
}

const IActor* ActorRegistryImpl::TryGet(ActorHandle handle) const noexcept {
    return const_cast<ActorRegistryImpl*>(this)->TryGet(handle);
}

IActor* ActorRegistryImpl::TryGetByGuid(const WorldGuid& guid) noexcept {
    std::shared_lock lock(m_Mutex);
    const auto it = m_ByGuid.find(guid);
    if (it == m_ByGuid.end()) {
        return nullptr;
    }
    return TryGet(it->second);
}

IActor* ActorRegistryImpl::TryGetByEntityId(std::uint64_t entityId) noexcept {
    std::shared_lock lock(m_Mutex);
    const auto it = m_ByEntity.find(entityId);
    if (it == m_ByEntity.end()) {
        return nullptr;
    }
    return TryGet(it->second);
}

IActor* ActorRegistryImpl::TryGetByName(std::string_view name) noexcept {
    std::shared_lock lock(m_Mutex);
    for (const ActorHandle& h : m_All) {
        if (auto* a = TryGet(h); a && a->Name() == name) {
            return a;
        }
    }
    return nullptr;
}

std::span<const ActorHandle> ActorRegistryImpl::All() const noexcept { return m_All; }
std::size_t ActorRegistryImpl::Count() const noexcept { return m_All.size(); }

void ActorRegistryImpl::FlushPendingBeginPlay() {
    std::vector<ActorHandle> pending;
    {
        std::shared_lock lock(m_Mutex);
        for (const ActorHandle& h : m_All) {
            if (auto* a = dynamic_cast<ActorImpl*>(TryGet(h)); a && a->pendingBeginPlay) {
                pending.push_back(h);
            }
        }
    }
    for (ActorHandle h : pending) {
        if (auto* a = dynamic_cast<ActorImpl*>(TryGet(h))) {
            a->BeginPlay();
        }
    }
}

void ActorRegistryImpl::FlushPendingEndPlay() {
    std::vector<ActorHandle> pending;
    {
        std::shared_lock lock(m_Mutex);
        for (const ActorHandle& h : m_All) {
            if (auto* a = dynamic_cast<ActorImpl*>(TryGet(h)); a && a->pendingEndPlay) {
                pending.push_back(h);
            }
        }
    }
    for (ActorHandle h : pending) {
        Destroy(h, EndPlayReason::Destroyed);
    }
}

HierarchyServiceImpl::HierarchyServiceImpl(WorldImpl& world) : m_World(world) {}

bool HierarchyServiceImpl::SetParent(ActorHandle child, ActorHandle parent) {
    ActorImpl* c = m_World.GetActor(child);
    if (!c) {
        return false;
    }
    if (parent.IsValid() && IsAncestorOf(child, parent)) {
        return false; // cycle
    }

    Detach(child);
    c->parent = parent;
    if (ActorImpl* p = m_World.GetActor(parent)) {
        p->children.push_back(child);
    }

    if (LevelImpl* level = m_World.GetLevel(c->level); level && level->scene) {
        ecs::Entity childEntity{c->entityId};
        auto& ecsWorld = level->scene->World();
        auto& hier = ecsWorld.GetOrAdd<ecs::HierarchyComponent>(childEntity);
        if (ActorImpl* p = m_World.GetActor(parent)) {
            hier.parent = ecs::Entity{p->entityId};
        } else {
            hier.parent = {};
        }
    }

    WorldDiagnostics::Get().OnHierarchyOp();
    return true;
}

bool HierarchyServiceImpl::Detach(ActorHandle child) {
    ActorImpl* c = m_World.GetActor(child);
    if (!c) {
        return false;
    }
    if (ActorImpl* p = m_World.GetActor(c->parent)) {
        p->children.erase(std::remove(p->children.begin(), p->children.end(), child), p->children.end());
    }
    c->parent = {};
    WorldDiagnostics::Get().OnHierarchyOp();
    return true;
}

ActorHandle HierarchyServiceImpl::GetParent(ActorHandle actor) const noexcept {
    const ActorImpl* a = m_World.GetActor(actor);
    return a ? a->parent : ActorHandle{};
}

std::span<const ActorHandle> HierarchyServiceImpl::GetChildren(ActorHandle actor) const {
    const ActorImpl* a = m_World.GetActor(actor);
    if (!a) {
        m_ChildScratch.clear();
        return m_ChildScratch;
    }
    return a->children;
}

bool HierarchyServiceImpl::IsAncestorOf(ActorHandle ancestor, ActorHandle descendant) const noexcept {
    ActorHandle cur = descendant;
    while (cur.IsValid()) {
        if (cur == ancestor) {
            return true;
        }
        cur = GetParent(cur);
    }
    return false;
}

std::uint16_t HierarchyServiceImpl::Depth(ActorHandle actor) const noexcept {
    std::uint16_t depth = 0;
    ActorHandle cur = GetParent(actor);
    while (cur.IsValid()) {
        ++depth;
        cur = GetParent(cur);
        if (depth > 4096) {
            break;
        }
    }
    return depth;
}

void HierarchyServiceImpl::TraverseDepthFirst(ActorHandle root, const VisitFn& visit) const {
    if (!visit || !root.IsValid()) {
        return;
    }
    visit(root);
    const ActorImpl* a = m_World.GetActor(root);
    if (!a) {
        return;
    }
    for (ActorHandle child : a->children) {
        TraverseDepthFirst(child, visit);
    }
}

void HierarchyServiceImpl::TraverseBreadthFirst(ActorHandle root, const VisitFn& visit) const {
    if (!visit || !root.IsValid()) {
        return;
    }
    std::queue<ActorHandle> q;
    q.push(root);
    while (!q.empty()) {
        ActorHandle cur = q.front();
        q.pop();
        visit(cur);
        if (const ActorImpl* a = m_World.GetActor(cur)) {
            for (ActorHandle child : a->children) {
                q.push(child);
            }
        }
    }
}

SceneGraphImpl::SceneGraphImpl(WorldImpl& world) : m_World(world) {}

ActorHandle SceneGraphImpl::Root() const noexcept { return m_Root; }
IHierarchyService& SceneGraphImpl::Hierarchy() noexcept { return m_World.m_Hierarchy; }

std::span<const ActorHandle> SceneGraphImpl::Roots() const {
    m_Roots.clear();
    for (ActorHandle h : m_World.m_Actors.All()) {
        if (const ActorImpl* a = m_World.GetActor(h); a && !a->parent.IsValid()) {
            m_Roots.push_back(h);
        }
    }
    return m_Roots;
}

void SceneGraphImpl::Rebuild() {
    auto roots = Roots();
    m_Root = roots.empty() ? ActorHandle{} : roots.front();
}

TransformHierarchyImpl::TransformHierarchyImpl(WorldImpl& world) : m_World(world) {}

void TransformHierarchyImpl::SetLocalPosition(ActorHandle actor, const Vec3f& position) {
    if (ActorImpl* a = m_World.GetActor(actor)) {
        a->localPosition = position;
        MarkDirty(actor);
    }
}

void TransformHierarchyImpl::SetLocalRotationEuler(ActorHandle actor, const Vec3f& eulerDegrees) {
    if (ActorImpl* a = m_World.GetActor(actor)) {
        a->localRotation = eulerDegrees;
        MarkDirty(actor);
    }
}

void TransformHierarchyImpl::SetLocalScale(ActorHandle actor, const Vec3f& scale) {
    if (ActorImpl* a = m_World.GetActor(actor)) {
        a->localScale = scale;
        MarkDirty(actor);
    }
}

Vec3f TransformHierarchyImpl::GetLocalPosition(ActorHandle actor) const {
    const ActorImpl* a = m_World.GetActor(actor);
    return a ? a->localPosition : Vec3f{};
}

Vec3f TransformHierarchyImpl::GetLocalRotationEuler(ActorHandle actor) const {
    const ActorImpl* a = m_World.GetActor(actor);
    return a ? a->localRotation : Vec3f{};
}

Vec3f TransformHierarchyImpl::GetLocalScale(ActorHandle actor) const {
    const ActorImpl* a = m_World.GetActor(actor);
    return a ? a->localScale : Vec3f{1.f, 1.f, 1.f};
}

Vec3f TransformHierarchyImpl::GetWorldPosition(ActorHandle actor) const {
    Vec3f pos = GetLocalPosition(actor);
    ActorHandle parent = m_World.m_Hierarchy.GetParent(actor);
    while (parent.IsValid()) {
        const Vec3f pp = GetLocalPosition(parent);
        pos.x += pp.x;
        pos.y += pp.y;
        pos.z += pp.z;
        parent = m_World.m_Hierarchy.GetParent(parent);
    }
    return pos;
}

void TransformHierarchyImpl::MarkDirty(ActorHandle actor) {
    if (ActorImpl* a = m_World.GetActor(actor)) {
        if (LevelImpl* level = m_World.GetLevel(a->level); level && level->scene) {
            if (auto* t = level->scene->World().TryGet<ecs::TransformComponent>(ecs::Entity{a->entityId})) {
                t->dirty = true;
            }
        }
    }
}

void TransformHierarchyImpl::UpdateWorldTransforms() {
    // Hierarchy sum is computed on demand in GetWorldPosition; SyncToEcs materializes.
    SyncToEcs();
}

void TransformHierarchyImpl::SyncToEcs() {
    for (ActorHandle h : m_World.m_Actors.All()) {
        ActorImpl* a = m_World.GetActor(h);
        if (!a) {
            continue;
        }
        LevelImpl* level = m_World.GetLevel(a->level);
        if (!level || !level->scene) {
            continue;
        }
        if (auto* t = level->scene->World().TryGet<ecs::TransformComponent>(ecs::Entity{a->entityId})) {
            t->localPosition = {a->localPosition.x, a->localPosition.y, a->localPosition.z};
            t->localRotation = {a->localRotation.x, a->localRotation.y, a->localRotation.z};
            t->localScale = {a->localScale.x, a->localScale.y, a->localScale.z};
            t->dirty = true;
        }
    }
}

void TransformHierarchyImpl::SyncFromEcs() {
    for (ActorHandle h : m_World.m_Actors.All()) {
        ActorImpl* a = m_World.GetActor(h);
        if (!a) {
            continue;
        }
        LevelImpl* level = m_World.GetLevel(a->level);
        if (!level || !level->scene) {
            continue;
        }
        if (const auto* t = level->scene->World().TryGet<ecs::TransformComponent>(ecs::Entity{a->entityId})) {
            a->localPosition = {t->localPosition.x, t->localPosition.y, t->localPosition.z};
            a->localRotation = {t->localRotation.x, t->localRotation.y, t->localRotation.z};
            a->localScale = {t->localScale.x, t->localScale.y, t->localScale.z};
        }
    }
}

TagSystemImpl::TagSystemImpl(WorldImpl& world) : m_World(world) {}

TagId TagSystemImpl::RegisterTag(std::string_view name) {
    const std::string key(name);
    if (const auto it = m_NameToId.find(key); it != m_NameToId.end()) {
        return it->second;
    }
    if (m_NextBit == 0) {
        return {};
    }
    TagId id{m_NextBit};
    m_NextBit <<= 1;
    m_NameToId.emplace(key, id);
    m_IdToName.emplace(id.value, key);
    return id;
}

TagId TagSystemImpl::FindTag(std::string_view name) const noexcept {
    const auto it = m_NameToId.find(std::string(name));
    return it == m_NameToId.end() ? TagId{} : it->second;
}

std::string_view TagSystemImpl::TagName(TagId id) const noexcept {
    const auto it = m_IdToName.find(id.value);
    return it == m_IdToName.end() ? std::string_view{} : std::string_view(it->second);
}

void TagSystemImpl::AddTag(ActorHandle actor, TagId tag) {
    if (ActorImpl* a = m_World.GetActor(actor)) {
        a->tags.value |= tag.value;
    }
}

void TagSystemImpl::RemoveTag(ActorHandle actor, TagId tag) {
    if (ActorImpl* a = m_World.GetActor(actor)) {
        a->tags.value &= ~tag.value;
    }
}

bool TagSystemImpl::HasTag(ActorHandle actor, TagId tag) const noexcept {
    const ActorImpl* a = m_World.GetActor(actor);
    return a && (a->tags.value & tag.value) == tag.value;
}

TagId TagSystemImpl::GetTags(ActorHandle actor) const noexcept {
    const ActorImpl* a = m_World.GetActor(actor);
    return a ? a->tags : TagId{};
}

std::span<const ActorHandle> TagSystemImpl::QueryTag(TagId tag) const {
    m_QueryScratch.clear();
    for (ActorHandle h : m_World.m_Actors.All()) {
        if (HasTag(h, tag)) {
            m_QueryScratch.push_back(h);
        }
    }
    return m_QueryScratch;
}

LayerSystemImpl::LayerSystemImpl(WorldImpl& world) : m_World(world) {
    RegisterLayer("Default");
}

LayerId LayerSystemImpl::RegisterLayer(std::string_view name) {
    const std::string key(name);
    if (const auto it = m_NameToId.find(key); it != m_NameToId.end()) {
        return it->second;
    }
    LayerId id{m_NextId++};
    m_NameToId.emplace(key, id);
    m_IdToName.emplace(id.value, key);
    m_Visibility[id.value] = true;
    return id;
}

LayerId LayerSystemImpl::FindLayer(std::string_view name) const noexcept {
    const auto it = m_NameToId.find(std::string(name));
    return it == m_NameToId.end() ? LayerId{} : it->second;
}

std::string_view LayerSystemImpl::LayerName(LayerId id) const noexcept {
    const auto it = m_IdToName.find(id.value);
    return it == m_IdToName.end() ? std::string_view{} : std::string_view(it->second);
}

void LayerSystemImpl::SetLayer(ActorHandle actor, LayerId layer) {
    if (ActorImpl* a = m_World.GetActor(actor)) {
        a->layer = layer;
    }
}

LayerId LayerSystemImpl::GetLayer(ActorHandle actor) const noexcept {
    const ActorImpl* a = m_World.GetActor(actor);
    return a ? a->layer : LayerId{};
}

std::span<const ActorHandle> LayerSystemImpl::QueryLayer(LayerId layer) const {
    m_QueryScratch.clear();
    for (ActorHandle h : m_World.m_Actors.All()) {
        if (GetLayer(h) == layer) {
            m_QueryScratch.push_back(h);
        }
    }
    return m_QueryScratch;
}

void LayerSystemImpl::SetLayerVisible(LayerId layer, bool visible) {
    m_Visibility[layer.value] = visible;
}

bool LayerSystemImpl::IsLayerVisible(LayerId layer) const noexcept {
    const auto it = m_Visibility.find(layer.value);
    return it == m_Visibility.end() ? true : it->second;
}

QuerySystemImpl::QuerySystemImpl(WorldImpl& world) : m_World(world) {}

std::vector<ActorHandle> QuerySystemImpl::Query(const ActorQueryFilter& filter) const {
    WorldDiagnostics::Get().OnQuery();
    std::vector<ActorHandle> out;
    for (ActorHandle h : m_World.m_Actors.All()) {
        const ActorImpl* a = m_World.GetActor(h);
        if (!a || a->lifecycle == ActorLifecycle::Destroyed) {
            continue;
        }
        if (filter.level.IsValid() && a->level != filter.level) {
            continue;
        }
        if (filter.layer.IsValid() && a->layer != filter.layer) {
            continue;
        }
        if (filter.requiredTags.IsValid() && (a->tags.value & filter.requiredTags.value) != filter.requiredTags.value) {
            continue;
        }
        if (filter.excludedTags.IsValid() && (a->tags.value & filter.excludedTags.value) != 0) {
            continue;
        }
        if (filter.requireBegunPlay && a->lifecycle != ActorLifecycle::Playing) {
            continue;
        }
        out.push_back(h);
    }
    return out;
}

std::vector<ActorHandle> QuerySystemImpl::QueryByName(std::string_view name) const {
    WorldDiagnostics::Get().OnQuery();
    std::vector<ActorHandle> out;
    for (ActorHandle h : m_World.m_Actors.All()) {
        if (const ActorImpl* a = m_World.GetActor(h); a && a->Name() == name) {
            out.push_back(h);
        }
    }
    return out;
}

std::vector<ActorHandle> QuerySystemImpl::QueryWhere(const ActorPredicate& predicate) const {
    WorldDiagnostics::Get().OnQuery();
    std::vector<ActorHandle> out;
    if (!predicate) {
        return out;
    }
    for (ActorHandle h : m_World.m_Actors.All()) {
        if (predicate(h)) {
            out.push_back(h);
        }
    }
    return out;
}

ActorHandle QuerySystemImpl::FindFirst(const ActorQueryFilter& filter) const {
    auto results = Query(filter);
    return results.empty() ? ActorHandle{} : results.front();
}

SpatialQueryImpl::SpatialQueryImpl(WorldImpl& world) : m_World(world) {}

void SpatialQueryImpl::Rebuild() {
    m_Entries.clear();
    for (ActorHandle h : m_World.m_Actors.All()) {
        Entry e;
        e.actor = h;
        e.position = m_World.m_Transforms.GetWorldPosition(h);
        m_Entries.push_back(e);
    }
}

std::vector<ActorHandle> SpatialQueryImpl::Query(const SpatialQueryParams& params) const {
    WorldDiagnostics::Get().OnSpatialQuery();
    if (m_Entries.empty()) {
        const_cast<SpatialQueryImpl*>(this)->Rebuild();
    }

    auto matchesFilter = [&](ActorHandle h) {
        ActorQueryFilter f = params.filter;
        auto results = m_World.m_Queries.Query(f);
        return std::find(results.begin(), results.end(), h) != results.end() ||
               (!f.requiredTags.IsValid() && !f.excludedTags.IsValid() && !f.layer.IsValid() &&
                   !f.level.IsValid() && !f.requireBegunPlay);
    };

    std::vector<ActorHandle> out;
    for (const Entry& e : m_Entries) {
        if (out.size() >= params.maxResults) {
            break;
        }
        bool hit = false;
        if (params.shape == SpatialQueryShape::Sphere || params.shape == SpatialQueryShape::Point) {
            const float dx = e.position.x - params.center.x;
            const float dy = e.position.y - params.center.y;
            const float dz = e.position.z - params.center.z;
            const float r = params.shape == SpatialQueryShape::Point ? 0.f : params.radius;
            hit = (dx * dx + dy * dy + dz * dz) <= (r * r);
        } else if (params.shape == SpatialQueryShape::Box) {
            hit = e.position.x >= params.center.x - params.extents.x &&
                  e.position.x <= params.center.x + params.extents.x &&
                  e.position.y >= params.center.y - params.extents.y &&
                  e.position.y <= params.center.y + params.extents.y &&
                  e.position.z >= params.center.z - params.extents.z &&
                  e.position.z <= params.center.z + params.extents.z;
        } else {
            const float dx = e.position.x - params.center.x;
            const float dy = e.position.y - params.center.y;
            const float dz = e.position.z - params.center.z;
            hit = (dx * dx + dy * dy + dz * dz) <= (params.radius * params.radius);
        }
        if (hit && matchesFilter(e.actor)) {
            out.push_back(e.actor);
        }
    }
    return out;
}

std::vector<ActorHandle> SpatialQueryImpl::OverlapSphere(const Sphere3f& sphere, const ActorQueryFilter& filter) const {
    SpatialQueryParams params{};
    params.shape = SpatialQueryShape::Sphere;
    params.center = sphere.center;
    params.radius = sphere.radius;
    params.filter = filter;
    return Query(params);
}

std::vector<ActorHandle> SpatialQueryImpl::OverlapBox(const Aabb3f& box, const ActorQueryFilter& filter) const {
    SpatialQueryParams params{};
    params.shape = SpatialQueryShape::Box;
    params.center = {
        (box.min.x + box.max.x) * 0.5f,
        (box.min.y + box.max.y) * 0.5f,
        (box.min.z + box.max.z) * 0.5f};
    params.extents = {
        (box.max.x - box.min.x) * 0.5f,
        (box.max.y - box.min.y) * 0.5f,
        (box.max.z - box.min.z) * 0.5f};
    params.filter = filter;
    return Query(params);
}

TickSystemImpl::TickSystemImpl(WorldImpl& world) : m_World(world) {}

std::uint64_t TickSystemImpl::Register(const TickRegistration& reg, TickFunction fn) {
    Entry e;
    e.id = m_NextId++;
    e.reg = reg;
    e.fn = std::move(fn);
    m_Entries.push_back(std::move(e));
    std::stable_sort(m_Entries.begin(), m_Entries.end(), [](const Entry& a, const Entry& b) {
        if (a.reg.group != b.reg.group) {
            return static_cast<std::uint8_t>(a.reg.group) < static_cast<std::uint8_t>(b.reg.group);
        }
        return a.reg.priority > b.reg.priority;
    });
    return e.id;
}

bool TickSystemImpl::Unregister(std::uint64_t tickId) {
    const auto it = std::remove_if(m_Entries.begin(), m_Entries.end(),
        [tickId](const Entry& e) { return e.id == tickId; });
    if (it == m_Entries.end()) {
        return false;
    }
    m_Entries.erase(it, m_Entries.end());
    return true;
}

void TickSystemImpl::SetFixedDeltaSeconds(float seconds) { m_FixedDt = seconds > 0.f ? seconds : m_FixedDt; }
float TickSystemImpl::FixedDeltaSeconds() const noexcept { return m_FixedDt; }
double TickSystemImpl::Accumulator() const noexcept { return m_Accumulator; }

void TickSystemImpl::Tick(IWorldContext& context, const WorldTickParams& params) {
    const auto t0 = std::chrono::steady_clock::now();

    if (params.runBeginPlay) {
        FlushBeginPlay(context);
    }

    auto runGroup = [&](TickGroup group, float dt) {
        for (Entry& e : m_Entries) {
            if (e.reg.group == group && e.fn) {
                e.fn(context, dt);
            }
        }
    };

    if (m_World.m_RuntimeDeps.physicsHook) {
        m_World.m_RuntimeDeps.physicsHook->OnPrePhysicsTick(context, params.deltaSeconds);
    }
    runGroup(TickGroup::PrePhysics, params.deltaSeconds);
    runGroup(TickGroup::DuringPhysics, params.deltaSeconds);
    runGroup(TickGroup::PostPhysics, params.deltaSeconds);
    if (m_World.m_RuntimeDeps.physicsHook) {
        m_World.m_RuntimeDeps.physicsHook->OnPostPhysicsTick(context, params.deltaSeconds);
    }

    runGroup(TickGroup::PreUpdate, params.deltaSeconds);

    if (params.runFixedUpdate) {
        m_Accumulator += params.deltaSeconds;
        const float fixedDt = params.fixedDeltaSeconds > 0.f ? params.fixedDeltaSeconds : m_FixedDt;
        int safety = 0;
        while (m_Accumulator >= fixedDt && safety++ < 8) {
            runGroup(TickGroup::DuringUpdate, fixedDt);
            m_Accumulator -= fixedDt;
            WorldDiagnostics::Get().OnFixedTick();
        }
    } else {
        runGroup(TickGroup::DuringUpdate, params.deltaSeconds);
    }

    runGroup(TickGroup::PostUpdate, params.deltaSeconds);
    runGroup(TickGroup::PreLateUpdate, params.deltaSeconds);
    runGroup(TickGroup::DuringLateUpdate, params.deltaSeconds);
    runGroup(TickGroup::PostLateUpdate, params.deltaSeconds);

    if (m_World.m_RuntimeDeps.networkHook) {
        m_World.m_RuntimeDeps.networkHook->OnNetTick(context, params.deltaSeconds);
    }
    if (m_World.m_RuntimeDeps.renderHook) {
        m_World.m_RuntimeDeps.renderHook->OnExtractFrame(context);
    }

    FlushEndPlay(context);

    const auto t1 = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    WorldDiagnostics::Get().OnTick(ms);
}

void TickSystemImpl::FlushBeginPlay(IWorldContext&) {
    m_World.m_Actors.FlushPendingBeginPlay();
}

void TickSystemImpl::FlushEndPlay(IWorldContext&) {
    m_World.m_Actors.FlushPendingEndPlay();
}

void WorldSubsystemRegistryImpl::Register(std::unique_ptr<IWorldSubsystem> subsystem) {
    if (!subsystem) {
        return;
    }
    m_Ptrs.push_back(subsystem.get());
    m_Owned.push_back(std::move(subsystem));
}

bool WorldSubsystemRegistryImpl::Unregister(std::string_view name) {
    for (std::size_t i = 0; i < m_Owned.size(); ++i) {
        if (m_Owned[i] && m_Owned[i]->Name() == name) {
            m_Owned[i]->Deinitialize();
            m_Ptrs.erase(m_Ptrs.begin() + static_cast<std::ptrdiff_t>(i));
            m_Owned.erase(m_Owned.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
    }
    return false;
}

IWorldSubsystem* WorldSubsystemRegistryImpl::Find(std::string_view name) noexcept {
    for (IWorldSubsystem* s : m_Ptrs) {
        if (s && s->Name() == name) {
            return s;
        }
    }
    return nullptr;
}

std::span<IWorldSubsystem* const> WorldSubsystemRegistryImpl::All() const { return m_Ptrs; }

void WorldSubsystemRegistryImpl::InitializeAll(IWorld& world) {
    for (IWorldSubsystem* s : m_Ptrs) {
        if (s) s->Initialize(world);
    }
}

void WorldSubsystemRegistryImpl::DeinitializeAll() {
    for (auto it = m_Ptrs.rbegin(); it != m_Ptrs.rend(); ++it) {
        if (*it) (*it)->Deinitialize();
    }
}

void WorldSubsystemRegistryImpl::BeginPlayAll(IWorldContext& context) {
    for (IWorldSubsystem* s : m_Ptrs) {
        if (s) s->BeginPlay(context);
    }
}

void WorldSubsystemRegistryImpl::EndPlayAll(IWorldContext& context, EndPlayReason reason) {
    for (auto it = m_Ptrs.rbegin(); it != m_Ptrs.rend(); ++it) {
        if (*it) (*it)->EndPlay(context, reason);
    }
}

} // namespace we::runtime::world::detail

namespace we::runtime::world {

WorldSubsystemFactoryRegistry& WorldSubsystemFactoryRegistry::Get() noexcept {
    static WorldSubsystemFactoryRegistry instance;
    return instance;
}

void WorldSubsystemFactoryRegistry::Register(std::string name, WorldSubsystemFactory factory) {
    if (!factory) {
        return;
    }
    for (Entry& e : m_Entries) {
        if (e.name == name) {
            e.factory = factory;
            return;
        }
    }
    m_Entries.push_back(Entry{std::move(name), factory});
}

bool WorldSubsystemFactoryRegistry::Unregister(std::string_view name) {
    const auto it = std::remove_if(m_Entries.begin(), m_Entries.end(),
        [&](const Entry& e) { return e.name == name; });
    if (it == m_Entries.end()) {
        return false;
    }
    m_Entries.erase(it, m_Entries.end());
    return true;
}

std::vector<std::unique_ptr<IWorldSubsystem>> WorldSubsystemFactoryRegistry::CreateAll() const {
    std::vector<std::unique_ptr<IWorldSubsystem>> out;
    out.reserve(m_Entries.size());
    for (const Entry& e : m_Entries) {
        if (e.factory) {
            if (auto s = e.factory()) {
                out.push_back(std::move(s));
            }
        }
    }
    return out;
}

} // namespace we::runtime::world
