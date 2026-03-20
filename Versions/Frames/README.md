# Version Frames

Incremental delta frames are stored here. Each frame records only the
changes (additions, mutations) that occurred between two consecutive
snapshots. Used by `Core::VersionSystem` for efficient history traversal
and by AI agents for training on code evolution patterns.
