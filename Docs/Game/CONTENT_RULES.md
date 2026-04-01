# NovaForge Game — Content Rules

## Content Layout
```text
Content/
  Data/
  Definitions/
  Buildables/
  Rig/
  Salvage/
  Worldgen/
  Factions/
  Seasons/
  Localization/
  Prototypes/
  SaveSchemas/
```

## Rules
- content ids must be stable and unique
- content references must use ids
- all major content schemas should carry a version field
- prototype content must be clearly isolated
- release validation should reject unresolved references
- save-sensitive schema changes must be version-aware
