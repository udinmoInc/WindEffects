#pragma once

namespace we::runtime::ecs {

template <typename T, typename... Args>
T& World::AddComponent(Entity entity, Args&&... args) {
    const ComponentTypeId typeId = ComponentTypeRegistry::Get().Register<T>();
    const ComponentTypeInfo* info = ComponentTypeRegistry::Get().Find(typeId);
    if (info && info->category == ComponentCategory::Singleton) {
        return SetSingleton<T>(std::forward<Args>(args)...);
    }
    if (Has<T>(entity)) {
        return Get<T>(entity);
    }

    T value(std::forward<Args>(args)...);
    MigrateAddComponent(entity, typeId, &value);
    BumpChangeVersion();
    return Get<T>(entity);
}

template <typename T>
bool World::RemoveComponent(Entity entity) {
    if (!Has<T>(entity)) {
        return false;
    }
    const ComponentTypeId typeId = ComponentTypeRegistry::Get().Id<T>();
    MigrateRemoveComponent(entity, typeId);
    BumpChangeVersion();
    return true;
}

template <typename T, typename... Args>
T& World::GetOrAdd(Entity entity, Args&&... args) {
    if (T* existing = TryGet<T>(entity)) {
        return *existing;
    }
    return AddComponent<T>(entity, std::forward<Args>(args)...);
}

template <typename T>
T* World::TryGet(Entity entity) {
    if (!Valid(entity)) {
        return nullptr;
    }
    const ComponentTypeId typeId = ComponentTypeRegistry::Get().Id<T>();
    const ComponentTypeInfo* info = ComponentTypeRegistry::Get().Find(typeId);
    if (info && info->category == ComponentCategory::Singleton) {
        return TryGetSingleton<T>();
    }
    ChunkView view = GetChunkView(entity);
    if (!view.Valid() || !view.Archetype()->mask.Test(typeId)) {
        return nullptr;
    }
    const EntityLocation& loc = Entities().Location(entity);
    return &view.Get<T>(loc.slot);
}

template <typename T>
const T* World::TryGet(Entity entity) const {
    return const_cast<World*>(this)->TryGet<T>(entity);
}

template <typename T>
T& World::Get(Entity entity) {
    T* ptr = TryGet<T>(entity);
    if (!ptr) {
        static T fallback{};
        return fallback;
    }
    return *ptr;
}

template <typename T>
const T& World::Get(Entity entity) const {
    return const_cast<World*>(this)->Get<T>(entity);
}

template <typename T>
bool World::Has(Entity entity) const {
    if (!Valid(entity)) {
        return false;
    }
    const ComponentTypeId typeId = ComponentTypeRegistry::Get().Id<T>();
    const ComponentTypeInfo* info = ComponentTypeRegistry::Get().Find(typeId);
    if (info && info->category == ComponentCategory::Singleton) {
        return TryGetSingleton<T>() != nullptr;
    }
    const EntityLocation& loc = Entities().Location(entity);
    if (loc.archetypeIndex == kInvalidArchetypeIndex) {
        return false;
    }
    const ArchetypeLayout* archetype = Archetypes().Get(loc.archetypeIndex);
    return archetype && archetype->mask.Test(typeId);
}

template <typename T>
void World::SetComponentEnabled(Entity entity, bool enabled) {
    if (!Valid(entity)) {
        return;
    }
    const ComponentTypeId typeId = ComponentTypeRegistry::Get().Id<T>();
    ChunkView view = GetChunkView(entity);
    if (!view.Valid()) {
        return;
    }
    const EntityLocation& loc = Entities().Location(entity);
    std::uint8_t* enabledColumn = static_cast<std::uint8_t*>(view.ColumnPtr(typeId));
    if (!enabledColumn) {
        return;
    }
    enabledColumn[loc.slot] = enabled ? 1 : 0;
    BumpChangeVersion();
}

template <typename T>
bool World::IsComponentEnabled(Entity entity) const {
    if (!Valid(entity)) {
        return false;
    }
    const ComponentTypeId typeId = ComponentTypeRegistry::Get().Id<T>();
    const EntityLocation& loc = Entities().Location(entity);
    if (loc.archetypeIndex == kInvalidArchetypeIndex) {
        return false;
    }
    const ArchetypeLayout* archetype = Archetypes().Get(loc.archetypeIndex);
    if (!archetype || !archetype->mask.Test(typeId)) {
        return false;
    }
    Chunk* chunk = archetype->chunks[loc.chunkIndex];
    ChunkView view(archetype, chunk);
    return view.IsComponentEnabled(typeId, loc.slot);
}

template <typename T, typename... Args>
T& World::SetSingleton(Args&&... args) {
    const ComponentTypeId typeId = ComponentTypeRegistry::Get().Register<T>();
    auto& storage = Singletons()[typeId];
    storage.resize(sizeof(T));
    new (storage.data()) T(std::forward<Args>(args)...);
    BumpChangeVersion();
    return *reinterpret_cast<T*>(storage.data());
}

template <typename T>
T* World::TryGetSingleton() {
    const ComponentTypeId typeId = ComponentTypeRegistry::Get().Id<T>();
    auto it = Singletons().find(typeId);
    if (it == Singletons().end() || it->second.size() < sizeof(T)) {
        return nullptr;
    }
    return reinterpret_cast<T*>(it->second.data());
}

template <typename T>
const T* World::TryGetSingleton() const {
    return const_cast<World*>(this)->TryGetSingleton<T>();
}

} // namespace we::runtime::ecs
