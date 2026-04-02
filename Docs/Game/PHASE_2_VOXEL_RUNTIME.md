# Phase 2 — Voxel Runtime Only

## Goal
Make voxels the authoritative world structure.

## Required Runtime Blocks
- voxel type/id registry
- chunk coordinate math
- chunk container and storage
- chunk dirty state tracking
- voxel edit API
- chunk serialization format
- chunk streaming policy
- mining/damage hook points
- repair hook points
- editor voxel visibility toggle
- editor voxel inspect/edit tool path

## Recommended Initial Constraints
- start with one chunk size and lock it
- use deterministic indexing rules
- keep meshing and rendering secondary to data correctness
- validate chunk bounds on every write in debug builds

## Definition of Done
- chunks load and unload deterministically
- voxels can be added and removed safely
- save/load preserves exact voxel state
- mining and damage hooks fire against voxel truth
- editor can inspect voxel state in the dev world
