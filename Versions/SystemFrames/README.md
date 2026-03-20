# System Frames

Engine subsystem state frames. Records the configuration and runtime state of
Core subsystems (EventBus subscriptions, TaskSystem queue depth, PluginSystem
registry, etc.) at each snapshot boundary. Used for reproducible replay and
AI-assisted debugging of system-level issues.
