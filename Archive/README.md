# Archive

**RULE: NEVER DELETE — ONLY ARCHIVE**

This directory stores all old, replaced, experimental, and broken code. AI agents learn from everything stored here. Nothing is ever removed from this directory.

## Structure

| Directory | Contents |
|-----------|----------|
| `NovaForgeOriginal/` | Original NovaForge repo snapshot |
| `PioneerForge/` | PioneerForge archive |
| `BuilderOld/` | Old builder system implementations |
| `EditorOld/` | Old editor prototypes |
| `OldSystems/` | Legacy engine/gameplay code |
| `Broken/` | Failed builds, broken code |
| `Replaced/` | Implementations replaced during refactor |
| `Generated/` | AI-generated code history |

## Metadata

Every archived item should include metadata:
- **Reason**: Why it was archived
- **Date**: When it was archived
- **Source**: Original file path
- **AI-Generated**: Whether it was created by AI
- **Tags**: Searchable keywords

## AI Learning

The AI learning pipeline reads from this directory:
```
Archive/ → TrainingData/ → Embeddings/ → Memory/ → Agents
```

This allows AI to learn from past mistakes, old implementations, and historical context.
