# NOVAFORGE_ROLLUP_INDEX.md

## Purpose
This package rolls up the key artifacts generated in the chat so the work can continue in a fresh window without losing the implementation trail.

## Included artifacts
- `patchset/COMPILE_READY_PATCH_SET.md`
- `NovaForge_compile_ready_patch_set.zip`
- `patchset2/SECOND_PATCH_SET_DOCK_LAYOUT_AND_INSPECTOR.md`
- `NovaForge_second_patch_set_dock_layout_and_inspector.zip`
- `EDITOR_CONTROL_AND_PHASE_RECOVERY_PLAN.md`
- `patchset3/THIRD_PATCH_SET_VIEWPORT_CLIP_WORLDSTATE.md`
- `NovaForge_third_patch_set_viewport_clip_worldstate.zip`
- `EDITOR_CONTROL_SCHEMA_AND_ASSET_MODEL_EDITOR.md`
- `EDITOR_COMMANDS_AND_TOOL_MODES.md`
- `scaffold_pack/SCAFFOLD_PACK_README.md`
- `NovaForge_scaffold_pack_commands_tools_paths.zip`
- `integration_patch_set/INTEGRATION_PATCH_SET_README.md`
- `integration_patch_set/PATCH_NOTES.md`
- `NovaForge_integration_patch_set_commands_tools_paths.zip`
- `MISSING_SYSTEMS_MATRIX.md`
- `NovaForge_missing_systems_compile_ready_pack.zip`
- `editor_core_scaffold/README_SELECTION_PROPERTY_OUTLINER.md`
- `NovaForge_selection_property_outliner_scaffold.zip`
- `selection_property_outliner_integration/README_SELECTION_PROPERTY_OUTLINER_INTEGRATION.md`
- `NovaForge_selection_property_outliner_integration_patch.zip`
- `runtime_outliner_property_sync_patch/README_RUNTIME_OUTLINER_PROPERTY_SYNC.md`
- `NovaForge_runtime_outliner_property_sync_patch.zip`

## Missing artifacts
- None

## Recommended implementation order
1. First patch set
2. Second patch set
3. Third patch set
4. Commands/tools/paths scaffold pack
5. Commands/tools/paths integration patch
6. Missing systems compile-ready pack
7. Selection/property/outliner scaffold
8. Selection/property/outliner integration patch
9. Runtime outliner/property/highlight sync patch

## Primary docs to read first
- `EDITOR_CONTROL_AND_PHASE_RECOVERY_PLAN.md`
- `EDITOR_CONTROL_SCHEMA_AND_ASSET_MODEL_EDITOR.md`
- `EDITOR_COMMANDS_AND_TOOL_MODES.md`
- `MISSING_SYSTEMS_MATRIX.md`
- `GITHUB_COPILOT_IMPLEMENTATION_DIRECTIONS.md`

## Notes
- Some packs are scaffolds. They define interfaces and ownership, not full runtime wiring.
- Use the latest patch touching a system when conflicts appear.
- Replace stub implementations with real repo bindings as you integrate.
