# Changelog

All notable changes to this project are documented in this file. The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and follows semantic versioning.

---

## [1.0.0] - 2026-07-05

### Development Milestone

This entry documents the state of the project as of 2026-07-05. WindEffects Engine is still under active development and has no official or public releases yet.

### Added

| Feature | Description |
|---------|-------------|
| **Benchmark Command** | Performance measurement and analysis across scenarios: cold builds, warm builds, no-op builds, target-specific builds, unity builds, and parallelism variations. Automated scenario testing for consistent, reproducible measurements. |
| **Multi-Format Reports** | Benchmark reports in JSON (programmatic analysis), CSV (spreadsheet import), Markdown (documentation), and HTML (web presentation). Includes timing, resource utilization, and cache effectiveness data. |
| **Consolidated Diagnostics** | Health checks for cache integrity/effectiveness, database consistency/performance, compiler pool health/utilization, SDK availability/configuration, and daemon operational status. Single entry point for system health assessment with actionable guidance. |

### Changed

| Change | Description |
|--------|-------------|
| **Version Alignment** | Version aligned to 1.0.0 across assembly, CLI, and documentation for consistent version identity. Applies to compiled binaries, CLI version, and all documentation. |
| **Dependency Removal** | Removed unused external command-line parsing dependency; retained hand-rolled CLI parser. Reduces external dependencies and compatibility issues. |
| **Performance Tracking** | Benchmark suite writes `latest.md` for trend comparison across runs, enabling regression/improvement detection over time. |

### Removed

| Removed Component | Reason |
|-------------------|--------|
| **Duplicate Distributed Interfaces** | Canonical stubs retained for distributed build interface contracts; redundant implementations removed. |
| **Duplicate Build Tuning** | Scheduler implementation retained; duplicate tuning component removed. |
| **Compiler Daemon Stack** | Unused experimental distributed compilation feature removed. |
| **Legacy Components** | JSON database replaced with SQLite; polling scheduler replaced with event-driven scheduler; legacy SDK detector replaced with robust detection system. |
| **Unused Helpers** | Compiler include parser, unity file generator, link optimizer, memory-mapped hasher removed (unused or superseded). |
| **Memory Utilities** | Arena allocator, object pool, string intern pool, async log sink removed (not used in production code path). |
| **Dead Orchestrator Wiring** | Persistent graph cache and header impact analyzer removed (not consumed on hot path; artifact-level caching and current PCH strategy are more effective). |

### Fixed

| Fix | Description |
|-----|-------------|
| **Diagnostics Compile Error** | Fixed undefined diagnostic layout preventing diagnostics command compilation. Removed duplicate diagnostic sections for clean output. |