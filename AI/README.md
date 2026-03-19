# AI

AI brain and learning systems. All models run **offline** via Ollama / llama.cpp.

## Modules

| Module | Description | Status |
|--------|-------------|--------|
| `Models/` | Local LLM model files (.gguf, .bin) | Phase 5 |
| `Memory/` | Long-term AI memory storage | Phase 5 |
| `Embeddings/` | Code/asset embeddings for search | Phase 5 |
| `Training/` | Training data processing | Phase 5 |
| `Context/` | Session context management | Phase 5 |
| `Prompts/` | Prompt templates and library | Phase 5 |
| `ArchiveLearning/` | Learning from archived code | Phase 5 |
| `CodeLearning/` | Learning from live codebase | Phase 5 |
| `AssetLearning/` | Learning from asset patterns | Phase 5 |
| `ErrorLearning/` | Learning from build errors | Phase 5 |
| `ModelManager/` | Model loading and switching | Phase 5 |
| `AgentScheduler/` | Multi-agent task scheduling | Phase 5 |

## Learning Pipeline

```
Archive + Projects + Runtime + Builder + Logs + Versions
    → TrainingData
    → Embeddings
    → Memory
    → Agents
```

## Rules

- **Offline only** — no cloud dependency
- **Sandbox** — AI never modifies live project directly
- **1-hour sessions** — iteration sessions capped, then report results
- **Never delete** — all AI outputs archived for learning
