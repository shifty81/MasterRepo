# GITHUB_COPILOT_IMPLEMENTATION_DIRECTIONS.md

## Purpose
Use this package as the implementation baseline for the NovaForge standalone editor and runtime stabilization effort.

This package contains:
- design docs
- action matrices
- scaffold packs
- integration patch packs
- hardening patch packs

Your job is to merge them into the repo in the order below, resolve conflicts in favor of the latest architectural direction, and keep the editor/runtime split clean.

---

## Non-negotiable project rules

- NovaForge is a standalone repo.
- Editor code must not leak into the shipping game executable.
- Custom UI only. Do not introduce ImGui.
- Voxel-first runtime remains the priority.
- DevWorld is the fixed sandbox for early testing.
- Prefer minimal working vertical slices over broad fake coverage.
- CMake should remain the source of truth for build structure.

---

## Implementation order

### Phase A — Stabilize the current shell
Apply these first:

1. `NovaForge_compile_ready_patch_set.zip`
2. `NovaForge_second_patch_set_dock_layout_and_inspector.zip`
3. `NovaForge_third_patch_set_viewport_clip_worldstate.zip`

Goals:
- resize/layout truth
- dock rect truth
- viewport sub-rect rendering
- clip/scissor correction
- honest world-state logging
- inspector wiring improvements

Validation:
- editor maximizes correctly
- viewport stays inside docked bounds
- no dead regions after maximize
- logs show actual world readiness

---

### Phase B — Add command/path/tool scaffolds
Apply these next:

4. `NovaForge_scaffold_pack_commands_tools_paths.zip`
5. `NovaForge_integration_patch_set_commands_tools_paths.zip`

Goals:
- `ProjectPathService`
- `EditorCommandRegistry`
- `MenuBarPanel`
- `ToolbarPanel`
- `EditorToolContext`
- `VoxelPickService` integration seam
- editor boot path logging

Critical requirement:
Replace any blind relative path usage with resolved paths from `ProjectPathService`.

Validation:
- log executable/current path/repo root/manifest/DevWorld path
- editor resolves `Config/novaforge.project.json`
- editor resolves `Content/Definitions/DevWorld.json`
- command shell is created from one registry

---

### Phase C — Add missing shell systems
Apply these next:

6. `NovaForge_missing_systems_compile_ready_pack.zip`

Goals:
- `WorldFileService`
- `DevWorldSerializer`
- `EditorHotkeyMap`
- `StatusBarPanel`

Critical requirement:
Upgrade `DevWorldSerializer` from scaffold file IO to the repo's actual JSON library/parser.

Validation:
- save/load/reload command handlers call real world services
- hotkeys dispatch into `EditorCommandRegistry`
- status bar reflects real editor state

---

### Phase D — Add selection/inspector/outliner foundations
Apply these next:

7. `NovaForge_selection_property_outliner_scaffold.zip`
8. `NovaForge_selection_property_outliner_integration_patch.zip`
9. `NovaForge_runtime_outliner_property_sync_patch.zip`

Goals:
- `SelectionService`
- `PropertyInspectorSystem`
- `WorldOutlinerPanel`
- runtime chunk metadata contract
- property dirty tracking
- viewport highlight sync
- outliner selection sync

Validation:
- content browser selection updates inspector and status bar
- viewport selection updates inspector and outliner
- outliner selection updates viewport highlight state
- property edits mark dirty state

---

## Merge rules

When files conflict:
- prefer the newest patch that touches the same system
- keep compile-safe seams if live runtime binding is not ready yet
- do not reintroduce placeholder-only behavior if a later patch added explicit contracts

Priority by recency:
1. runtime/outliner/property sync patch
2. selection/property/outliner integration patch
3. missing systems pack
4. integration patch set
5. scaffold pack
6. third patch set
7. second patch set
8. first patch set

---

## What Copilot should implement for real

The packs include scaffolds. Replace these scaffold seams with real repo bindings:

### Path resolution
- query the real executable directory on Windows
- walk upward to find `Config/novaforge.project.json`
- fail loudly if not found

### DevWorld serialization
- parse and write real JSON
- preserve fixed DevWorld test fields
- ensure reload reproduces saved state

### World save/load
- bind `WorldFileService` to actual runtime world lifecycle
- support load, reload, save, save-as

### Voxel picking
- replace stub `VoxelPickService` logic with:
  - camera ray build
  - viewport-local coordinates
  - chunk traversal
  - voxel hit result
  - adjacent placement cell

### Chunk metadata
- feed live chunk ids, coordinates, dirty flags, meshed flags into the outliner

### Chunk edit loop
- voxel add/remove must:
  - mark chunk dirty
  - rebuild mesh
  - upload render data
  - set world dirty
  - refresh status and outliner state

### Menu and toolbar UI
- render real clickable controls in the custom UI system
- wire them to `EditorCommandRegistry`

### Inspector widgets
- render actual bool/int/float/string/vec3 controls
- push edits through `PropertyInspectorSystem`
- mark editor/world dirty as appropriate

### Viewport highlights
- draw visible highlight overlays for selected chunk/voxel/object
- keep selection in sync with outliner and inspector

---

## Must-have editor loop

The editor is not considered healthy until this loop works:

1. Load DevWorld
2. Show visible world
3. Select asset/object/chunk/voxel
4. Inspect properties
5. Edit something
6. Dirty state becomes visible
7. Rebuild/render updates
8. Save world
9. Reload world
10. State is preserved correctly

---

## Must-have technical checks

### Build and output
- confirm editor output path
- confirm game output path
- do not assume `Debug/` root
- verify actual executable target names

### Editor boot truth
At startup log:
- executable path
- current working directory
- repo root
- manifest path
- content root
- DevWorld path
- file existence results

### Runtime truth
Never log "world loaded" before:
- world definition exists
- initial chunks exist
- meshing/upload path ran
- viewport can represent the world as non-empty

---

## Systems still future-needed after this package

Do not block current implementation on these, but keep the architecture open for them:
- session persistence
- world outliner expansion
- property reflection framework
- asset database/import pipeline
- undo/redo transaction model
- notification system
- validation framework
- DevWorld settings panel
- voxel palette/brush panel
- play/test bridge

---

## Expected repo structure direction

Keep or align toward:
- `Source/Editor/...`
- `Source/Game/...`
- `Source/Core/...`
- `Content/...`
- `Config/...`
- `Docs/...`

Do not merge editor-only concerns into runtime/game-only modules.

---

## Working style for Copilot

- Implement one subsystem at a time.
- Prefer small compile-safe commits.
- Keep logging explicit and factual.
- When a scaffold exists, replace internals before redesigning the interface.
- Do not broaden scope beyond DevWorld + voxel runtime stabilization until the core loop works.
