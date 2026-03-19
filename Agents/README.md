# Agents

AI agent implementations. Each agent specializes in a specific task domain.

## Agent List

| Agent | Description | Status |
|-------|-------------|--------|
| `CodeAgent/` | Code generation from prompts | Phase 5 |
| `FixAgent/` | Bug fixing and compile error resolution | Phase 5 |
| `BuildAgent/` | Build automation and optimization | Phase 5 |
| `CleanupAgent/` | Workspace cleanup and organization | Phase 5 |
| `AssetAgent/` | Asset generation from text prompts | Phase 9 |
| `PCGAgent/` | Procedural content generation | Phase 7 |
| `EditorAgent/` | Editor task automation | Phase 8 |
| `RefactorAgent/` | Code refactoring | Phase 5 |
| `DebugAgent/` | Debug assistance | Phase 5 |
| `VersionAgent/` | Version management | Phase 5 |

## Agent Architecture

Each agent follows the same core loop:

```
Read prompt → Send to LLM → Parse response → Execute tools → Return result
```

Agents can:
- Read/write files
- Run compilers
- Execute scripts
- Generate code
- Fix errors
- Chain with other agents
