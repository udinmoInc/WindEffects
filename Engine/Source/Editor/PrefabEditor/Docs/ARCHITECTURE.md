# Prefab Editor Architecture

> PrefabEditor is the editor binding layer for Undo-aware prefab commands and session access. Prefab Runtime remains the authority for assets, instances, and overrides.

## DI

1. Editor creates `CreatePrefabRuntime` then `CreatePrefabEditor` with `recordTransaction` → `Undo::RecordCustom(..., TransactionKind::Prefab, ...)`.
2. `PrefabSession::Install` exposes runtime to Viewport/Outliner/Content Browser consumers.
3. Drag-drop: payload `prefab` / `prefab-guid` / `asset` → `SpawnFromPayload`.

## Undo

Every successful `ExecuteCommand` records an undo/redo pair. Spawn undo despawns the latest instance of that asset; create-from-selection undo unregisters the path.
