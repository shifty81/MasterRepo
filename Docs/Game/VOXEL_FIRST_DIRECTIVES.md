# NovaForge Voxel-First Directives

## Status
This document locks current implementation priority for NovaForge.

## Core Rule
Implementation begins with the voxel layer first.
The full low-poly overlay is deferred until the voxel structure and tooling are stable.

## Voxel Layer Rules
- Voxel layer defines structure.
- Voxel layer defines mining, repair, destruction, and procedural generation.
- Voxel layer should normally be invisible to the player during gameplay.
- In the editor, voxel visibility must be toggleable for inspection and editing.
- Construction granularity target is `32 x 32 x 32`.
- Generated skeletons and item shapes should originate from voxel structure.

## Low-Poly Layer Rules
- Low-poly is the visible wrapper layer.
- Low-poly should closely wrap the voxel structure.
- Low-poly should represent voxel damage states rather than replace the voxel logic.
- Low-poly collision can be used later if it remains a near-exact wrapper, but
  voxel truth stays authoritative for structure.

## Rig Rules
- The player suit is called a `rig`.
- Starting state is a minimal exo rig / backpack frame.
- Life support is built into the rig platform, but begins in a limited state.
- The starter torso/back rig is the first frame segment and first armor state.
- Helmet deploys from the back rig assembly and enables minimal HUD.
- Arm modules unlock expanded HUD capability and can enable backpack crafting.

## Implementation Sequence
1. voxel runtime and chunk rules
2. editor voxel inspection and authoring tools
3. rig starter-state gameplay integration
4. damage/mining/repair loop
5. low-poly wrap planning
6. low-poly visual collision wrapper only after voxel fidelity is trusted
