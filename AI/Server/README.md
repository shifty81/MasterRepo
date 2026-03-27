# Atlas AI Server

A standalone local AI web server for MasterRepo. Opens a **ChatGPT-style browser interface** at `http://localhost:8080` that is fully context-aware of the entire project folder via deep indexing.

Refactored from [Arbiter](https://github.com/shifty81/Arbiter)'s FastAPI bridge architecture and adapted for MasterRepo's C++ game-engine project.

---

## Quick Start

```bash
# Windows
start.bat

# Linux / macOS / Git Bash
./start.sh

# Manual
pip install fastapi uvicorn[standard]
python atlas_ai_server.py --project /path/to/MasterRepo
```

Then open **http://localhost:8080** in your browser.

---

## What You Can Do

| Action | How |
|--------|-----|
| Ask about any file, system, or architecture | Just type in the chat |
| Read a specific file | Type `/read Engine/Core/Engine.h` |
| Browse files | Sidebar file tree — click to view |
| Re-index the project | Click **↻ Re-index Project** in the sidebar |
| Plan new features | Ask naturally — full project context is injected |
| Generate code snippets | Ask with context ("add a new ECS component") |

---

## Requirements

- Python 3.10+
- [Ollama](https://ollama.ai) running locally: `ollama serve`
- A pulled model: `ollama pull codellama`

---

## API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| `GET`  | `/` | Web UI |
| `GET`  | `/api/status` | Server + Ollama + index status |
| `POST` | `/api/chat` | Chat with Ollama (project context injected) |
| `GET`  | `/api/history` | Get chat history |
| `DELETE` | `/api/history` | Clear chat history |
| `POST` | `/api/index` | Trigger re-index |
| `GET`  | `/api/structure` | Project tree + summary |
| `GET`  | `/api/files?path=` | List files in a directory |
| `GET`  | `/api/file?path=` | Read a file's content |

---

## Command-line Options

```
python atlas_ai_server.py [options]

  --port      HTTP port (default: 8080)
  --host      Bind host (default: 127.0.0.1)
  --project   Path to MasterRepo root (auto-detected)
  --model     Ollama model name (default: codellama)
  --ollama-port  Ollama port (default: 11434)
```

---

## Architecture

```
AI/Server/
├── atlas_ai_server.py   FastAPI server — chat, file ops, index endpoints
├── project_indexer.py   Deep project scanner (C++ symbol extraction, context builder)
├── static/
│   └── index.html       ChatGPT-style single-page web UI
├── requirements.txt     Python dependencies
├── start.sh             One-click launch (Linux/macOS/Git Bash)
└── start.bat            One-click launch (Windows)
```

The project indexer walks the entire repo on startup, extracts C++ namespaces/classes/functions, and builds a focused context string injected into every Ollama prompt — so the AI knows the full project without you having to explain it.
