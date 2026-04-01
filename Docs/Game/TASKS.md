# NovaForge Game — Tasks

## Completed (imported from NovaForge-Project)
- [x] Establish game module structure (`Source/Game/`)
- [x] Import design documentation (`Docs/Game/`)
- [x] Define project manifest (`novaforge.project.json`)
- [x] Define game architecture and module dependency graph

## Game Module Buildout
- [ ] `NF::Game::App` — Bootstrap, Session, ProjectContext, Orchestrator
- [ ] `NF::Game::World` — GameWorld facade over NF::Level
- [ ] `NF::Game::App::Diagnostics` — RuntimeDiagnostics JSON reporter
- [ ] `NF::Game::App::Playtest` — PlaytestSession + TestHarness

## Gameplay Systems
- [ ] implement world and voxel primitives
- [ ] implement player R.I.G. baseline
- [ ] implement building and salvage baseline
- [ ] implement season authority
- [ ] implement client and server startup
- [ ] implement validation suite
- [ ] implement packaging and release pipeline

## CI / Build
- [ ] wire smoke-test playtest session into CI
- [ ] validate content schemas against CONTENT_RULES.md
