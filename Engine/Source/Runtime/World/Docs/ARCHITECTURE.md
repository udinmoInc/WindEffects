# 🌍 World Runtime Architecture

> The World Runtime is the permanent gameplay and editor foundation for WindEffects Engine. It orchestrates world, level, and actor lifetime on top of ECS storage, Reflection metadata, Serialization persistence, and Asset Runtime streaming — without duplicating those systems' responsibilities.

---

## 🌐 System Overview

```
┌──────────────────────────────────────────────────────────────────────────┐
│ Editor / Game / Dedicated Server / Plugins                                │
└──────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌──────────────────────────────────────────────────────────────────────────┐
│ IWorldRuntime  →  IWorldManager  →  IWorldRegistry                        │
│        │                                                                  │
│        └─ IWorld (per instance)                                           │
│              ├─ ILevel (persistent + streamed)                            │
│              ├─ IActorRegistry / IActor                                   │
│              ├─ ISceneGraph / IHierarchyService / ITransformHierarchy     │
│              ├─ ITagSystem / ILayerSystem                                 │
│              ├─ IQuerySystem / ISpatialQuery                              │
│              ├─ ITickSystem (groups + fixed update)                       │
│              ├─ IWorldStreamer / IWorldPartition                          │
│              ├─ IWorldPersistence / IPrefabInstancer / IOriginRebasing    │
│              └─ IWorldSubsystemRegistry (plugins)                         │
└──────────────────────────────────────────────────────────────────────────┘
                                    │
          ┌─────────────────────────┼─────────────────────────┐
          ▼                         ▼                         ▼
   scene::Scene              Reflection                 Serialization
   ecs::Registry / World     (type metadata)            (WEBN + WEWD)
          │
          ▼
   AssetRuntime (async stream foundation)
```

| Layer | Owns | Does not own |
|-------|------|--------------|
| 🌍 **World Runtime** | World/level/actor identity, lifecycle, tick, hierarchy policy, streaming policy, extension points | Component bytes, type schemas, asset bytes |
| 🧩 **Scene** | ECS-backed scene container, editor view projection | Multi-world orchestration |
| 🧠 **ECS** | Entity/component storage, systems, archetypes | Gameplay actor identity |
| 🔎 **Reflection** | Type metadata, serialize plans | Object graphs |
| 💾 **Serialization** | WEBN documents, async serialize | World lifetime |
| 📦 **Asset Runtime** | Cooked asset residency | World object graphs |

> **💡 Key Benefit:** New world features (partition strategies, query providers, actor factories, persistence backends) plug in through interfaces and registries — no edits to core World Runtime implementations required.

---

## 🧱 Ownership & Boundaries

### Lifetime

1. `CreateWorldRuntime(deps)` constructs `IWorldRuntime` with injected Reflection / Serialization / Asset / integration hooks.
2. `IWorldManager::CreateWorld` allocates an `IWorld`, creates a persistent `ILevel`, and optionally adopts an existing `scene::Scene`.
3. `IActorRegistry::Spawn` creates a Scene/ECS entity, then binds an `ActorHandle` + `WorldGuid`.
4. `Destroy` / `EndPlay` / world teardown reverse the chain deterministically.

### Update Order (deterministic)

```
BeginPlay flush
  → PrePhysics → DuringPhysics → PostPhysics
  → PreUpdate → DuringUpdate (fixed steps) → PostUpdate
  → PreLateUpdate → DuringLateUpdate → PostLateUpdate
  → EndPlay flush
  → Transform sync → Level Scene::Update → SceneGraph rebuild
```

Subsystem `Tick` runs before the tick scheduler groups so gameplay modules can enqueue work.

---

## 🔌 Extension Points

| Extension | Mechanism |
|-----------|-----------|
| World subsystems | `IWorldSubsystem` + `WorldSubsystemFactoryRegistry` |
| Physics / Audio / Net / Editor / Render | `I*WorldHook` injected via `WorldRuntimeDependencies` |
| Typed services | `IWorldContext::RegisterService<T>` |
| Prefab sources | `IPrefabInstancer::RegisterPrefabSource` |
| Partition policy | Replaceable `IWorldPartition` strategy (cell grid foundation) |
| Streaming | `IWorldStreamer` async load/unload |
| Reflection types | `WorldTypeRegistrar` / `RegisterWorldReflectionTypes` |

---

## 🧵 Threading Model

| Path | Policy |
|------|--------|
| Registry lookups | `shared_mutex` — concurrent readers |
| Actor spawn/destroy | Exclusive lock on actor registry |
| Tick | Single-threaded per world (deterministic); async tick callbacks opt-in via `TickRegistration::canTickAsync` for future job wiring |
| Save/Load/Stream | `std::async` workers; completion applied on caller/game thread |
| Diagnostics | Lock-free atomics |

No global mutable gameplay state. `EnvironmentSystem::Get()` remains a legacy environment helper for editor compatibility; new code should prefer `IWorld` / context services.

---

## 🔗 Integration Map

| System | Integration |
|--------|-------------|
| **ECS** | Each level owns/shares `scene::Scene` → `Registry` / `ecs::World` |
| **Reflection** | Descriptors registered at module/runtime startup |
| **Serialization** | Optional `ISerializer` bind; WEWD envelope + ECS capture foundation |
| **Asset Runtime** | Dependency reserved for cooked level/prefab streaming |
| **Renderer** | `IRenderWorldHook::OnExtractFrame`; Scene already produces `ExtractedFrameData` |
| **Physics / Audio / Net / Editor** | Hooks only — no hardcoded engine knowledge |

---

## 📁 Module Layout

```
Engine/Source/Runtime/World/
├── World.Build.cs
├── Docs/ARCHITECTURE.md
├── Public/World/          # stable interfaces + types
├── Public/Environment/    # legacy environment helpers
├── Public/DefaultScene/
└── Private/               # concrete implementations (detail::)
```

### Factory entry

```cpp
WorldRuntimeDependencies deps;
deps.typeRegistry = &reflection::GetTypeRegistry();
deps.serializer = serializer.get();
auto runtime = CreateWorldRuntime(deps);

WorldCreateInfo info{};
info.descriptor.persistent = true;
auto world = runtime->Manager().CreateWorld(info);
```

---

## ✅ Validation

- `RunWorldRuntimeTests()` — create/destroy, hierarchy, tags/layers, tick, queries, save/load, multi-world, streaming, prefab, origin rebase
- `RunWorldRuntimeBenchmarks()` — spawn, tick, hierarchy, query, spatial, persistence

---

## 🚀 Future Systems (no architectural redesign)

Property Editor, Viewport, Undo/Redo, Prefabs (full), Scene Editor, Animation, Physics, AI, Navigation, Networking, Multiplayer, Visual Scripting, Sequencer, Live Editing, World Partition (production), DLC, Plugins, Dedicated Server — all attach through existing interfaces, hooks, and subsystem factories.
