"""
atlas_ai_server.py — Standalone local AI web server for MasterRepo.

Refactored from Arbiter's fastapi_bridge.py. Run this to get a
ChatGPT-style browser interface at http://localhost:8080 that is
fully context-aware of the entire project folder.

Usage:
    python atlas_ai_server.py [--port 8080] [--project /path/to/MasterRepo]

Endpoints:
    GET  /               → ChatGPT-style web UI (index.html)
    GET  /api/status     → server + Ollama + index status
    POST /api/chat       → chat with Ollama (project context injected)
    GET  /api/index      → trigger re-index; return stats
    GET  /api/files      → list files in a directory
    GET  /api/file       → read a single file's content
    GET  /api/structure  → project tree overview
"""

import argparse
import asyncio
import json
import os
import sys
import threading
import time
import urllib.request
import urllib.error
from pathlib import Path
from typing import List, Optional

# ── FastAPI / Uvicorn ──────────────────────────────────────────────────────────
try:
    from fastapi import FastAPI, HTTPException
    from fastapi.middleware.cors import CORSMiddleware
    from fastapi.responses import HTMLResponse, JSONResponse, StreamingResponse
    from fastapi.staticfiles import StaticFiles
    from pydantic import BaseModel
    import uvicorn
except ImportError:
    print("[Atlas AI] Missing dependencies. Run:  pip install fastapi uvicorn[standard]")
    sys.exit(1)

# ── Project root resolution ────────────────────────────────────────────────────
_SERVER_DIR  = Path(__file__).parent.resolve()
_DEFAULT_ROOT = _SERVER_DIR.parent.parent   # AI/Server/../../ = repo root

# ── Local imports ──────────────────────────────────────────────────────────────
sys.path.insert(0, str(_SERVER_DIR))
from project_indexer import ProjectIndexer

# ── App setup ─────────────────────────────────────────────────────────────────
app = FastAPI(title="Atlas AI Server", version="1.0.0")
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# Static files (web UI lives in AI/Server/static/)
_static_dir = _SERVER_DIR / "static"
if _static_dir.is_dir():
    app.mount("/static", StaticFiles(directory=str(_static_dir)), name="static")

# ── Global state ──────────────────────────────────────────────────────────────
_indexer: Optional[ProjectIndexer] = None
_ollama_host = "localhost"
_ollama_port = 11434
_ollama_model = "codellama"
_chat_history: List[dict] = []   # {"role": "user"|"assistant", "content": str}

# ── Pydantic models ────────────────────────────────────────────────────────────
class ChatRequest(BaseModel):
    message: str
    model:   str = ""
    stream:  bool = False

class IndexRequest(BaseModel):
    force: bool = False

# ── Ollama helpers ─────────────────────────────────────────────────────────────
def _ollama_ping() -> bool:
    try:
        req = urllib.request.Request(
            f"http://{_ollama_host}:{_ollama_port}/api/tags",
            method="GET"
        )
        with urllib.request.urlopen(req, timeout=3):
            return True
    except Exception:
        return False

def _ollama_generate(prompt: str, system: str = "", model: str = "") -> str:
    """Blocking Ollama /api/generate call (stream:false)."""
    mdl = model or _ollama_model
    body = json.dumps({
        "model":  mdl,
        "prompt": prompt,
        "system": system,
        "stream": False,
    }).encode()
    try:
        req = urllib.request.Request(
            f"http://{_ollama_host}:{_ollama_port}/api/generate",
            data=body,
            headers={"Content-Type": "application/json"},
            method="POST"
        )
        with urllib.request.urlopen(req, timeout=120) as resp:
            raw = resp.read().decode()
        data = json.loads(raw)
        if "error" in data:
            err = data["error"]
            if "not found" in err:
                return f"⚠ Model not found: {err}\nRun:  ollama pull {mdl}"
            return f"⚠ Ollama error: {err}"
        return data.get("response", "")
    except urllib.error.HTTPError as e:
        try:
            error_body = e.read()
            err_data = json.loads(error_body.decode())
            err = err_data.get("error", str(e))
            if "not found" in err:
                return f"⚠ Model not found: {err}\nRun:  ollama pull {mdl}"
            return f"⚠ Ollama error: {err}"
        except Exception:
            return f"⚠ Ollama error: {e}"
    except urllib.error.URLError:
        return "⚠ Ollama not reachable. Start it with:  ollama serve"
    except Exception as e:
        return f"⚠ Error: {e}"

def _build_system_prompt() -> str:
    ctx = ""
    if _indexer and _indexer.is_ready():
        ctx = _indexer.build_context()
    return (
        "You are Atlas AI — the local development assistant for MasterRepo, "
        "an offline AI-powered C++ game engine + editor + game project.\n\n"
        "You have full context of the project structure below. "
        "Answer questions about architecture, code, planning, and implementation. "
        "Be concise but thorough. Use code blocks for code snippets.\n\n"
        + ctx
    )

# ── Routes ─────────────────────────────────────────────────────────────────────

@app.get("/", response_class=HTMLResponse)
async def root():
    """Serve the ChatGPT-style web UI."""
    html_path = _static_dir / "index.html"
    if html_path.is_file():
        return HTMLResponse(html_path.read_text(encoding="utf-8"))
    return HTMLResponse("<h2>Atlas AI Server running. Place index.html in AI/Server/static/</h2>")


@app.get("/api/status")
async def status():
    ollama_ok = _ollama_ping()
    idx = _indexer.get_index() if _indexer else None
    return {
        "server":        "Atlas AI Server v1.0",
        "ollama":        ollama_ok,
        "ollama_url":    f"http://{_ollama_host}:{_ollama_port}",
        "model":         _ollama_model,
        "index_ready":   _indexer is not None and _indexer.is_ready(),
        "indexed_files": idx.total_files if idx else 0,
        "indexed_lines": idx.total_lines if idx else 0,
        "indexed_at":    idx.indexed_at  if idx else None,
    }


@app.post("/api/chat")
async def chat(req: ChatRequest):
    """Send a message; get a response from Ollama with project context."""
    global _chat_history

    user_msg = req.message.strip()
    if not user_msg:
        raise HTTPException(400, "Empty message")

    # Check for file read command: /read path/to/file
    if user_msg.startswith("/read ") and _indexer:
        rel_path = user_msg[6:].strip()
        content = _indexer.read_file(rel_path)
        # Truncate at a line boundary to avoid cutting tokens mid-line
        if len(content) > 4000:
            truncated = content[:4000]
            last_nl = truncated.rfind('\n')
            if last_nl > 0:
                truncated = truncated[:last_nl]
            content = truncated + "\n... (truncated — use /read for full content)"
        assistant_msg = f"**Contents of `{rel_path}`:**\n```\n{content}\n```"
        _chat_history.append({"role": "user",      "content": user_msg})
        _chat_history.append({"role": "assistant", "content": assistant_msg})
        return {"reply": assistant_msg, "history_length": len(_chat_history)}

    # Normal chat — build context-aware prompt
    system  = _build_system_prompt()
    # Include last 6 turns of history for coherence
    history_ctx = ""
    for turn in _chat_history[-6:]:
        role = "User" if turn["role"] == "user" else "Assistant"
        history_ctx += f"{role}: {turn['content']}\n"

    full_prompt = history_ctx + f"User: {user_msg}\nAssistant:"

    # Run in thread pool to avoid blocking event loop
    loop  = asyncio.get_event_loop()
    model = req.model or _ollama_model
    reply = await loop.run_in_executor(
        None, _ollama_generate, full_prompt, system, model
    )

    _chat_history.append({"role": "user",      "content": user_msg})
    _chat_history.append({"role": "assistant", "content": reply})
    # Keep history bounded
    if len(_chat_history) > 100:
        _chat_history = _chat_history[-80:]

    return {"reply": reply, "history_length": len(_chat_history)}


@app.get("/api/history")
async def history():
    return {"history": _chat_history}


@app.delete("/api/history")
async def clear_history():
    global _chat_history
    _chat_history = []
    return {"ok": True}


@app.post("/api/index")
async def reindex(req: IndexRequest):
    """Trigger a re-index of the project folder."""
    if not _indexer:
        raise HTTPException(503, "Indexer not initialised")
    loop  = asyncio.get_event_loop()
    idx   = await loop.run_in_executor(None, _indexer.index, req.force)
    return {
        "ok":    True,
        "files": idx.total_files,
        "lines": idx.total_lines,
    }


@app.get("/api/structure")
async def structure():
    if not _indexer or not _indexer.is_ready():
        raise HTTPException(503, "Index not ready — call POST /api/index first")
    idx = _indexer.get_index()
    return {
        "tree":    idx.directory_tree,
        "summary": idx.summary,
    }


@app.get("/api/files")
async def list_files(path: str = ""):
    if not _indexer:
        raise HTTPException(503, "Indexer not initialised")
    return {"files": _indexer.list_files(path)}


@app.get("/api/file")
async def read_file(path: str):
    if not _indexer:
        raise HTTPException(503, "Indexer not initialised")
    content = _indexer.read_file(path)
    return {"path": path, "content": content}


# ── Startup event ─────────────────────────────────────────────────────────────
@app.on_event("startup")
async def on_startup():
    """Index the project asynchronously so the server is immediately responsive."""
    if _indexer:
        loop = asyncio.get_event_loop()
        loop.run_in_executor(None, _indexer.index, False)
        print("[Atlas AI] Background project indexing started…")


# ── Main entry point ──────────────────────────────────────────────────────────
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Atlas AI Server")
    parser.add_argument("--port",    type=int, default=8080,        help="HTTP port (default 8080)")
    parser.add_argument("--host",    default="127.0.0.1",           help="Bind host")
    parser.add_argument("--project", default=str(_DEFAULT_ROOT),    help="Project root path")
    parser.add_argument("--model",   default="codellama",           help="Ollama model name")
    parser.add_argument("--ollama-port", type=int, default=11434,   help="Ollama port")
    args = parser.parse_args()

    _ollama_model = args.model
    _ollama_port  = args.ollama_port

    _indexer = ProjectIndexer(args.project)
    print(f"[Atlas AI] Project root : {args.project}")
    print(f"[Atlas AI] Ollama model : {_ollama_model}  @ localhost:{_ollama_port}")
    print(f"[Atlas AI] Web UI       : http://{args.host}:{args.port}")
    print(f"[Atlas AI] Starting server…")

    uvicorn.run(app, host=args.host, port=args.port, log_level="warning")
