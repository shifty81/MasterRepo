# Phase 0 — Bootstrap

## Goal
Get the repo bootable, structurally clean, and safe to build against.

## Scope

### Engine
- core logging
- config loading
- platform bootstrap
- engine loop shell
- null/stub renderer path
- basic input shell
- world/level shell

### Editor
- editor app bootstrap
- dock/panel shell
- viewport shell
- project open path
- dev world selection path

### Game
- project context
- bootstrap sequence
- session lifecycle
- orchestrator shell
- diagnostics shell
- playtest shell

## Exact Targets
- `Source/Core/`
- `Source/Engine/`
- `Source/Editor/`
- `Source/Game/App/`
- `Source/Programs/NovaForgeEditor/`
- `Source/Programs/NovaForgeGame/`
- `Config/novaforge.project.json`

## Definition of Done
- configure succeeds from a clean checkout
- editor executable is produced
- game executable is produced
- editor enters its shell without crashing
- game enters its loop without crashing
- project paths resolve from manifest/config
- logging shows deterministic startup order
