# World Outliner Architecture

> World Outliner is the Editor’s authoritative hierarchy browser. It is a **presentation and interaction layer** over Scene (near-term) and World Runtime (foundation). It never owns gameplay data, ECS storage, reflection schemas, undo history, or viewport pixels.

---

## System Overview

```
ToolsPanel / Explorer Panel / Keyboard
              │
              ▼
IWorldOutlinerRuntime
       │
       ▼
IWorldOutliner
       ├─ IOutlinerTreeModel          (virtualized / filtered rows)
       ├─ IOutlinerDataProvider(s)    (Scene bridge + World Runtime stub)
       ├─ IOutlinerSelection
       ├─ IOutlinerSearch / Filter / Sorter
       ├─ IOutlinerFolderProvider
       ├─ IOutlinerCommandRouter      → Undo::RecordCustom
       ├─ IOutlinerDragDropHandler
       ├─ IOutlinerColumnProvider(s)  (Name/Type/Layer/Tag/Visibility/Lock + plugins)
       ├─ IOutlinerDecorator / ContextMenuProvider
       └─ IOutlinerEventRouter        ★ selection & hierarchy bus
                │
                ├─► Scene::SetSelectedEntityId
                ├─► ViewportEdit::Selection
                └─► PropertyEditor / Details (via Scene + Environment TickEditor)
```

| Layer | Owns | Does not own |
|-------|------|--------------|
| **WorldOutliner** | Tree model, filters, UI binding, commands, event bus | Actor/entity data, transforms, undo stack |
| **Scene / World** | Hierarchy & objects | Outliner UI |
| **Undo** | Transactions | Outliner widgets |
| **ViewportEdit** | Viewport selection / focus | Hierarchy browser |
| **PropertyEditor** | Details bindings | Tree structure |
| **Reflection / Serialization** | Metadata / persist | Outliner |

---

## Ownership & DI

1. Editor constructs `CreateWorldOutlinerRuntime(WorldOutlinerDependencies)` after Undo / PropertyEditor / ViewportEdit.
2. `WorldOutlinerSession::Install(runtime)` for panel factories.
3. Panel `BindTreeView` wires TreeView callbacks → CommandRouter / Selection (Undo-backed).
4. Environment keeps Details refresh on Scene selection; Outliner owns hierarchy rebuild and rename/reparent/delete.

---

## Selection Synchronization (no ad-hoc host sync)

`IOutlinerEventRouter` is the single publication point for selection changes.

| Source | Path |
|--------|------|
| Outliner click | Selection → EventRouter → Scene + ViewportEdit |
| Viewport pick | ViewportEdit selection → Outliner `SyncSelectionFromViewport` → Scene |
| Outliner / Viewport / Outliner | Scene id → Environment `TickEditor` → Details |

Hosts must not manually mirror selection between subsystems.

---

## Commands → Undo

| Command | TransactionKind |
|---------|-----------------|
| Rename | `Rename` |
| Reparent / DnD | `Reparent` |
| Delete | `ActorDestroy` / `ObjectDelete` |
| Duplicate | `ActorSpawn` |
| Create Folder | `ObjectCreate` |

Flow: Command → Scene/World mutation → `ITransactionManager::RecordCustom` → EventRouter → model rebuild → TreeView / Viewport / Details refresh.

---

## Data Providers

| Provider | Status |
|----------|--------|
| **SceneDataProvider** | Production bridge over `Scene::GetEntities` + `ParentId` |
| **WorldDataProvider** | Foundation stub for `IWorldRuntime` multi-world / levels / partition |

When Editor hosts `CreateWorldRuntime`, register World provider as primary; Scene provider remains compatibility adapter.

---

## Extension Points

| Extension | API |
|-----------|-----|
| Node sources | `RegisterDataProvider` |
| Columns | `RegisterColumn` |
| Filters | `RegisterFilter` |
| Decorators | `RegisterDecorator` |
| Context menus | `RegisterContextMenu` |
| Filter presets | `SaveFilterPreset` / `LoadFilterPreset` |

---

## Scalability

- Filtered visible-row list (not full paint of millions)
- Lazy child load flag on nodes (`OutlinerNodeFlags::lazyChildren`)
- Incremental `MarkDirty` / rebuild on hierarchy events
- Diagnostics: rebuild micros, node/visible counts, transaction counts

TreeView already virtualizes paint; Outliner model supplies filtered rows.

---

## Threading

Editor UI thread owns mutation and rebuild. Read-only diagnostics use atomics. Future worker rebuilds must publish via EventRouter on the UI thread.

---

## Constraints

- Never duplicate World/Scene hierarchy authority
- Every committed hierarchy edit records Undo
- Public headers are interface-first; ContentBrowser `TreeView` is a view adapter only
