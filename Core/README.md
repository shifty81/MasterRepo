# Core

Shared systems that **everything** depends on. This is the foundation layer.

## Modules

| Module | Description | Status |
|--------|-------------|--------|
| `Reflection/` | Runtime type info, property iteration | Phase 1 |
| `Metadata/` | Key-value metadata tagging | Phase 1 |
| `Serialization/` | JSON/binary save/load for reflected types | Phase 1 |
| `Messaging/` | Publish/subscribe message bus | Phase 1 |
| `Events/` | Typed event dispatching | Phase 1 |
| `TaskSystem/` | Multi-threaded job execution | Phase 1 |
| `CommandSystem/` | Undo/redo command pattern | Phase 1 |
| `ResourceManager/` | Asset loading and caching | Phase 1 |
| `ArchiveSystem/` | Archive management with metadata | Phase 1 |
| `VersionSystem/` | Snapshot/frame creation and diffing | Phase 1 |
| `PluginSystem/` | Dynamic plugin loading | Phase 1 |

## Dependencies

- Depends on: Nothing (root of dependency graph)
- Used by: Everything

## Build Order

This is built **first**, before all other modules.
