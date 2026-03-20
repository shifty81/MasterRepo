# Version Snapshots

Version snapshots are stored here as JSON files.
Format: `{version}_{timestamp}.json`

Each snapshot captures the complete project state at a point in time,
including all source file hashes, asset manifests, and build configuration.
Snapshots are written by `Core::VersionSystem` and are never deleted.
