# AI System

Atlas Engine's AI subsystem is a **fully offline, multi-agent architecture**
powered by local language models. No data leaves the machine.

---

## Overview

```
User / Editor
     │
     ▼
 Agent Core  ← plans tasks, selects tools, iterates
     │
     ├── LLM layer (Ollama / llama.cpp)
     ├── Tool runner (read file, write file, build, search …)
     ├── Code index (vector DB of repo symbols)
     └── Memory / context store (TrainingData/)
```

The Agent Core receives a goal, decomposes it into steps, selects tools to
execute each step, observes results, and iterates until the goal is satisfied
or a maximum step count is reached.

---

## Agent Types

| Agent | Responsibility |
|-------|---------------|
| `CodeAgent` | Generate and refactor C++ / Lua code to spec |
| `BuildAgent` | Run CMake, parse errors, retry until green |
| `FixAgent` | Receive a compiler error and produce a minimal fix |
| `RefactorAgent` | Rename symbols, extract functions, modernise code |
| `DocAgent` | Write or update documentation from code |
| `AssetAgent` | Call Blender addon to generate 3D assets |
| `PCGAgent` | Generate procedural content from a prompt |
| `TestAgent` | Write and run unit tests, report coverage |

Agents are registered in `Agents/` and share the same base class and tool set.

---

## Tool System

Each agent has access to a set of **tools** — thin wrappers that perform a
single, observable action:

| Tool | Description |
|------|-------------|
| `ReadFile(path)` | Read a file and return its contents |
| `WriteFile(path, content)` | Write or overwrite a file |
| `SearchCode(query)` | Semantic search over the code index |
| `RunBuild(config)` | Invoke CMake build; return stdout/stderr |
| `RunTests()` | Execute ctest; return pass/fail summary |
| `GitDiff()` | Return the current working-tree diff |
| `GitCommit(msg)` | Stage all changes and create a commit |
| `BlenderRender(spec)` | Call the Blender addon pipeline |
| `MemoryStore(k, v)` | Persist a key-value pair to the memory store |
| `MemoryRecall(k)` | Retrieve a stored memory by key |

Tools are registered at agent construction time and available as JSON function
calls in the LLM's output.

---

## Memory and Context System

Agents accumulate knowledge in two layers:

**Short-term context** — the current conversation / planning window held in
the LLM's context buffer. Capped by the model's context window size.

**Long-term memory** — key-value pairs written to `TrainingData/` via
`MemoryStore`. On next invocation an agent recalls relevant entries via
`MemoryRecall`. The memory store is also used to provide the LLM with
project-specific conventions (Atlas Rule, namespace conventions, etc.).

---

## Adding a New Agent

1. Create `Agents/MyAgent/MyAgent.h` and `MyAgent.cpp`.
2. Inherit the `Agents::BaseAgent` class (see `Agents/BaseAgent.h`).
3. Override `GetName()`, `GetDescription()`, and `RunGoal(goal, context)`.
4. Register the tools your agent needs in the constructor via `RegisterTool()`.
5. Add the agent to `Agents/CMakeLists.txt`.
6. Instantiate and register the agent with the agent registry at startup.

```cpp
class MyAgent : public Agents::BaseAgent {
public:
    std::string GetName()        const override { return "MyAgent"; }
    std::string GetDescription() const override { return "Does X"; }
    void        RunGoal(const std::string& goal,
                        Agents::AgentContext& ctx) override;
};
```

---

## Prompt Library

Reusable prompt templates live in `AI/Prompts/`. Each template is a plain text
file with `{{VARIABLE}}` placeholders that the agent fills at runtime.

Example template `AI/Prompts/fix_build_error.txt`:

```
You are a C++20 expert. The following build error occurred:
{{ERROR}}

The relevant source file is:
{{SOURCE}}

Produce a minimal fix. Output ONLY the corrected file content.
```

---

## Sandbox and Safety

All agent actions that write files or run processes are executed inside a
**sandbox layer** that:

- Restricts writes to the project directory tree (never `/etc`, `~`, etc.)
- Limits build invocations to a configurable timeout (default 120 s)
- Logs every tool call to `Archive/AgentLogs/` for audit and training
- Prevents deletion — write operations that would overwrite are logged as
  archives first via `Core::ArchiveSystem`
