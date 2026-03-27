#!/usr/bin/env bash
# start.sh — Launch the Atlas AI Server on Linux/macOS/Git-Bash
# Usage:  ./start.sh [--port 8080] [--model codellama]

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "=============================================="
echo "  Atlas AI Server — MasterRepo Assistant"
echo "  Project: $REPO_ROOT"
echo "=============================================="

# Check Python
if ! command -v python3 &>/dev/null && ! command -v python &>/dev/null; then
    echo "[ERROR] Python 3 not found. Install Python 3.10+ and retry."
    exit 1
fi

PY=python3
command -v python3 &>/dev/null || PY=python

# Create venv if missing
if [ ! -d "$SCRIPT_DIR/.venv" ]; then
    echo "[Setup] Creating virtual environment…"
    $PY -m venv "$SCRIPT_DIR/.venv"
fi

# Activate
if [ -f "$SCRIPT_DIR/.venv/Scripts/activate" ]; then
    # Windows Git-Bash
    source "$SCRIPT_DIR/.venv/Scripts/activate"
else
    source "$SCRIPT_DIR/.venv/bin/activate"
fi

# Install deps
pip install -q -r "$SCRIPT_DIR/requirements.txt"

# Check Ollama
if command -v ollama &>/dev/null; then
    echo "[Info]  Ollama found. Make sure 'ollama serve' is running."
else
    echo "[Warn]  Ollama not found. Start it separately: https://ollama.ai"
fi

echo "[Info]  Starting server at http://127.0.0.1:8080"
echo "[Info]  Open your browser to http://localhost:8080"
echo "[Info]  Press Ctrl+C to stop."
echo ""

cd "$SCRIPT_DIR"
python atlas_ai_server.py --project "$REPO_ROOT" "$@"
