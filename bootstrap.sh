#!/usr/bin/env bash
# ==============================================================================
# MasterRepo Bootstrap Script   (Windows — Git Bash / MSYS2)
# ==============================================================================
# Purpose:
#   - Verify required tools are installed
#   - Create project directory structure
#   - Set up Python virtual environment for AI/tooling
#   - Initialize default configuration files
#   - Prepare the workspace for development
#
# Requirements: Git Bash or MSYS2 on Windows 10/11
# ==============================================================================

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"

# ── Interactive detection (same pattern as other build scripts) ───────────────
_is_interactive() {
    [[ "${CI:-}"   == "true" ]] && return 1
    [[ "${TERM:-}" == "dumb" ]] && return 1
    [[ -t 0 && -t 1 ]]
}

# ── Logging ──────────────────────────────────────────────────────────────────
mkdir -p "${ROOT_DIR}/Logs/Build"
_BOOT_TS="$(date +%Y%m%d_%H%M%S)"
_BOOT_LOG="${ROOT_DIR}/Logs/Build/bootstrap_${_BOOT_TS}.log"

# Source shared logging library if available
if [[ -f "${ROOT_DIR}/Scripts/Tools/lib_log.sh" ]]; then
    # shellcheck source=Scripts/Tools/lib_log.sh
    source "${ROOT_DIR}/Scripts/Tools/lib_log.sh"
    log_init "bootstrap"
    _BOOT_LOG="$(log_file)"
fi

# Write all output to the log file directly (no tee pipe — avoids hang on Windows).
exec >> "${_BOOT_LOG}" 2>&1

echo "=== MasterRepo Bootstrap ==="
echo "Platform: Windows (Git Bash / MSYS2)"
echo "Project root: ${ROOT_DIR}"
echo "Log file: ${_BOOT_LOG}"

# ---------------------------
# 1. Check Required Tools
# ---------------------------
echo ""
echo "--- Checking required tools ---"

check_tool() {
    local tool="$1" desc="$2"
    if command -v "${tool}" &>/dev/null; then
        local _out _ver
        _out="$("${tool}" --version 2>&1 || true)"
        _ver="${_out%%$'\n'*}"; _ver="${_ver%%$'\r'*}"
        echo "  [OK] ${tool}: ${_ver}"
    else
        echo "  [!!] ${tool} not found — ${desc}"
    fi
}

check_tool git   "Required for version control"
check_tool cmake "Required for building (https://cmake.org)"
check_tool python "Required for AI tools and scripting"
check_tool pip   "Required for Python package management"

echo ""
echo "--- Checking optional tools ---"
check_tool node   "Optional: needed for Monaco IDE integration"
check_tool npm    "Optional: needed for Monaco IDE integration"
check_tool ollama "Optional: needed for local LLM support"

# ---------------------------
# 2. Create Directory Structure
# ---------------------------
echo ""
echo "--- Creating directory structure ---"

directories=(
    "Engine/Render" "Engine/Physics" "Engine/Audio" "Engine/Input"
    "Engine/Window" "Engine/Platform" "Engine/Math"
    "Core/Reflection" "Core/Metadata" "Core/Serialization" "Core/Messaging"
    "Core/Events" "Core/TaskSystem" "Core/CommandSystem" "Core/ResourceManager"
    "Core/ArchiveSystem" "Core/VersionSystem" "Core/PluginSystem"
    "Editor/Core" "Editor/UI" "Editor/Docking" "Editor/Panels"
    "Editor/Viewport" "Editor/Gizmos" "Editor/Overlay" "Editor/Modes"
    "Editor/NodeEditors" "Editor/BuilderEditor" "Editor/PCGEditor"
    "Editor/MaterialEditor" "Editor/Commands"
    "Runtime/World" "Runtime/ECS" "Runtime/Components" "Runtime/Systems"
    "Runtime/Gameplay" "Runtime/Player" "Runtime/Inventory" "Runtime/Equipment"
    "Runtime/Crafting" "Runtime/BuilderRuntime" "Runtime/Damage"
    "Runtime/Collision" "Runtime/Spawn" "Runtime/SaveLoad" "Runtime/UI"
    "AI/Models" "AI/Memory" "AI/Embeddings" "AI/Training" "AI/Context"
    "AI/Prompts" "AI/ArchiveLearning" "AI/CodeLearning" "AI/AssetLearning"
    "AI/ErrorLearning" "AI/ModelManager" "AI/AgentScheduler"
    "Agents/CodeAgent" "Agents/FixAgent" "Agents/BuildAgent"
    "Agents/CleanupAgent" "Agents/AssetAgent" "Agents/PCGAgent"
    "Agents/EditorAgent" "Agents/RefactorAgent" "Agents/DebugAgent"
    "Agents/VersionAgent"
    "Builder/Core" "Builder/Parts" "Builder/Modules" "Builder/PhysicsData"
    "Builder/Collision" "Builder/Damage" "Builder/Assembly" "Builder/SnapRules"
    "PCG/Geometry" "PCG/Textures" "PCG/Audio" "PCG/World"
    "PCG/Quests" "PCG/Rules" "PCG/Validation"
    "UI/Widgets" "UI/Layouts" "UI/Themes"
    "IDE/CodeEditor" "IDE/Console" "IDE/AIChat" "IDE/Debugger"
    "Tools/ServerManager" "Tools/AssetTools" "Tools/BuildTools" "Tools/Importer"
    "Tools/GUI/panels"
    "Projects/NovaForge/Assets" "Projects/NovaForge/Prefabs"
    "Projects/NovaForge/Parts" "Projects/NovaForge/Modules"
    "Projects/NovaForge/Ships" "Projects/NovaForge/Recipes"
    "Projects/NovaForge/Scenes" "Projects/NovaForge/UI"
    "Projects/NovaForge/Config"
    "Archive/NovaForgeOriginal" "Archive/PioneerForge" "Archive/BuilderOld"
    "Archive/EditorOld" "Archive/OldSystems" "Archive/Broken"
    "Archive/Replaced" "Archive/Generated"
    "WorkspaceState"
    "Versions/Frames" "Versions/Snapshots" "Versions/ObjectFrames"
    "Versions/SystemFrames" "Versions/SceneFrames"
    "TrainingData/Code" "TrainingData/Assets" "TrainingData/Errors"
    "TrainingData/Diffs"
    "External/SwissAgent" "External/AtlasForge" "External/NovaForgeOriginal"
    "Plugins/Editor" "Plugins/Runtime" "Plugins/AI" "Plugins/Builder"
    "Experiments/TestSystems" "Experiments/Prototypes" "Experiments/Temp"
    "Builds/debug" "Builds/release"
    "Logs/Build" "Logs/Agent" "Logs/AI" "Logs/Server"
    "Config"
    "Scripts/Lua" "Scripts/Python" "Scripts/Tools"
    "Docs/Architecture" "Docs/API" "Docs/Builder" "Docs/Editor"
    "Docs/AI" "Docs/Runtime" "Docs/Guides"
)

_dir_count=0
for dir in "${directories[@]}"; do
    mkdir -p "${ROOT_DIR}/${dir}"
    _dir_count=$(( _dir_count + 1 ))
done
echo "  Created ${_dir_count} directories"

# ---------------------------
# 3. Create Default Config Files
# ---------------------------
echo ""
echo "--- Creating default config files ---"

if [ ! -f "${ROOT_DIR}/Config/Engine.json" ]; then
cat > "${ROOT_DIR}/Config/Engine.json" << 'EOF'
{
    "window": {
        "title": "MasterRepo Engine",
        "width": 1920,
        "height": 1080,
        "fullscreen": false,
        "vsync": true
    },
    "renderer": {
        "backend": "DirectX12",
        "msaa": 4,
        "shadows": true,
        "maxFPS": 60
    },
    "physics": {
        "backend": "Bullet",
        "gravity": [0, -9.81, 0],
        "fixedTimestep": 0.016667
    },
    "audio": {
        "backend": "XAudio2",
        "masterVolume": 1.0
    }
}
EOF
echo "  Created Config/Engine.json"
fi

if [ ! -f "${ROOT_DIR}/Config/Editor.json" ]; then
cat > "${ROOT_DIR}/Config/Editor.json" << 'EOF'
{
    "theme": "dark",
    "fontSize": 14,
    "autosave": true,
    "autosaveInterval": 300,
    "layout": "default",
    "recentProjects": [],
    "showConsole": true,
    "showProfiler": false,
    "viewport": {
        "gridVisible": true,
        "gridSize": 1.0,
        "snapToGrid": true,
        "cameraMode": "orbit"
    },
    "gizmo": {
        "mode": "translate",
        "space": "world",
        "snapTranslate": 0.5,
        "snapRotate": 15.0,
        "snapScale": 0.1
    }
}
EOF
echo "  Created Config/Editor.json"
fi

if [ ! -f "${ROOT_DIR}/Config/AI.json" ]; then
cat > "${ROOT_DIR}/Config/AI.json" << 'EOF'
{
    "enabled": true,
    "offline": true,
    "modelProvider": "ollama",
    "defaultModel": "deepseek-coder",
    "models": {
        "code": "deepseek-coder",
        "chat": "llama3",
        "embedding": "nomic-embed-text"
    },
    "sessionTimeout": 3600,
    "maxTokens": 4096,
    "temperature": 0.7,
    "sandbox": true,
    "allowedPaths": [
        "Projects/",
        "Experiments/",
        "Scripts/"
    ],
    "memory": {
        "enabled": true,
        "maxEntries": 10000,
        "embeddingDimension": 384
    }
}
EOF
echo "  Created Config/AI.json"
fi

if [ ! -f "${ROOT_DIR}/Config/Builder.json" ]; then
cat > "${ROOT_DIR}/Config/Builder.json" << 'EOF'
{
    "snapDistance": 0.1,
    "weldStrength": 100.0,
    "maxPartsPerAssembly": 1000,
    "showSnapPoints": true,
    "showConnectionGraph": false,
    "physics": {
        "enableCollision": true,
        "enableDamage": true
    }
}
EOF
echo "  Created Config/Builder.json"
fi

if [ ! -f "${ROOT_DIR}/Config/Server.json" ]; then
cat > "${ROOT_DIR}/Config/Server.json" << 'EOF'
{
    "host": "0.0.0.0",
    "port": 7777,
    "maxPlayers": 32,
    "tickRate": 60,
    "networking": {
        "backend": "ENet",
        "timeout": 5000
    }
}
EOF
echo "  Created Config/Server.json"
fi

if [ ! -f "${ROOT_DIR}/Config/Projects.json" ]; then
cat > "${ROOT_DIR}/Config/Projects.json" << 'EOF'
{
    "activeProject": "NovaForge",
    "projects": {
        "NovaForge": {
            "path": "Projects/NovaForge",
            "description": "Primary game project — space exploration and construction",
            "version": "0.1.0"
        }
    }
}
EOF
echo "  Created Config/Projects.json"
fi

# ---------------------------
# 4. Python Virtual Environment
# ---------------------------
echo ""
echo "--- Setting up Python environment ---"

# On Windows, the Python launcher is 'python' (the 'py' alias also works).
# Git Bash / MSYS2 both honour the standard 'python' command when
# the Python installer's "Add to PATH" option was checked.
if command -v python &>/dev/null; then
    if [ ! -d "${ROOT_DIR}/venv" ]; then
        python -m venv "${ROOT_DIR}/venv"
        echo "  Created Python virtual environment: venv/"
        echo "  Activate with: source venv/Scripts/activate"
    else
        echo "  Python virtual environment already exists: venv/"
        echo "  Activate with: source venv/Scripts/activate"
    fi
else
    echo "  [!!] Python not found — skipping virtual environment setup"
    echo "       Install from https://www.python.org/downloads/ (check 'Add to PATH')"
fi

# ---------------------------
# 5. Summary
# ---------------------------
echo ""
echo "=== Bootstrap Complete ==="
echo ""
echo "Bootstrap log saved to: ${_BOOT_LOG}"
echo ""
echo "Next steps:"
echo "  1. Review ROADMAP.md for the full implementation plan"
echo "  2. Start with Phase 0 tasks (see ROADMAP.md)"
echo "  3. Build (from an x64 Native Tools Command Prompt or Git Bash):"
echo "       bash Scripts/Tools/build_all.sh"
echo "     Or manually:"
echo "       cmake -B Builds/debug -DCMAKE_BUILD_TYPE=Debug"
echo "       cmake --build Builds/debug --parallel %NUMBER_OF_PROCESSORS%"
echo ""
echo "Optional:"
echo "  - Install Ollama for local AI: https://ollama.com/download"
echo "    Then: ollama pull deepseek-coder"
echo "  - Activate Python env: source venv/Scripts/activate"
echo ""

# Finish logging if lib_log was sourced
if [[ -n "${_LIB_LOG_LOADED:-}" ]]; then
    log_finish
fi

# Only show the "Press Enter" prompt when running interactively
if _is_interactive; then
    printf "\nPress [Enter] to close...\n"
    read -r -p "" 2>/dev/null || true
fi
