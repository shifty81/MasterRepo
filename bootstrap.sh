#!/usr/bin/env bash
# ==============================================================================
# MasterRepo Bootstrap Script
# Multi-Platform (Linux / macOS / Windows via WSL or Git Bash)
# ==============================================================================
# Purpose:
#   - Verify required tools are installed
#   - Create project directory structure
#   - Set up Python virtual environment for AI/tooling
#   - Initialize default configuration files
#   - Prepare the workspace for development
# ==============================================================================

set -e

# ---------------------------
# 1. Detect OS
# ---------------------------
OS="$(uname -s)"
echo "=== MasterRepo Bootstrap ==="
echo "Detected OS: $OS"

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
echo "Project root: $ROOT_DIR"

# ---------------------------
# 2. Check Required Tools
# ---------------------------
echo ""
echo "--- Checking required tools ---"

check_tool() {
    if command -v "$1" &>/dev/null; then
        echo "  [OK] $1 found: $(command -v "$1")"
    else
        echo "  [!!] $1 not found — $2"
    fi
}

check_tool git "Required for version control"
check_tool cmake "Required for building the engine (install from https://cmake.org)"
check_tool python3 "Required for AI tools and scripting"
check_tool pip3 "Required for Python package management"

# Optional tools
echo ""
echo "--- Checking optional tools ---"
check_tool node "Optional: needed for Monaco IDE integration"
check_tool npm "Optional: needed for Monaco IDE integration"
check_tool blender "Optional: needed for Blender addon pipeline"
check_tool ollama "Optional: needed for local LLM support"

# ---------------------------
# 3. Create Directory Structure
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
    "Builds/Debug" "Builds/Release" "Builds/Editor"
    "Logs/Build" "Logs/Agent" "Logs/AI" "Logs/Server"
    "Config"
    "Scripts/Lua" "Scripts/Python/Blender" "Scripts/Tools"
    "Docs/Architecture" "Docs/API" "Docs/Builder" "Docs/Editor"
    "Docs/AI" "Docs/Runtime" "Docs/Guides"
)

for dir in "${directories[@]}"; do
    mkdir -p "$ROOT_DIR/$dir"
done

echo "  Created $(echo "${directories[@]}" | wc -w) directories"

# ---------------------------
# 4. Create Default Config Files
# ---------------------------
echo ""
echo "--- Creating default config files ---"

if [ ! -f "$ROOT_DIR/Config/Engine.json" ]; then
cat > "$ROOT_DIR/Config/Engine.json" << 'EOF'
{
    "window": {
        "title": "MasterRepo Engine",
        "width": 1920,
        "height": 1080,
        "fullscreen": false,
        "vsync": true
    },
    "renderer": {
        "backend": "OpenGL",
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
        "backend": "OpenAL",
        "masterVolume": 1.0
    }
}
EOF
echo "  Created Config/Engine.json"
fi

if [ ! -f "$ROOT_DIR/Config/Editor.json" ]; then
cat > "$ROOT_DIR/Config/Editor.json" << 'EOF'
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

if [ ! -f "$ROOT_DIR/Config/AI.json" ]; then
cat > "$ROOT_DIR/Config/AI.json" << 'EOF'
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

if [ ! -f "$ROOT_DIR/Config/Builder.json" ]; then
cat > "$ROOT_DIR/Config/Builder.json" << 'EOF'
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

if [ ! -f "$ROOT_DIR/Config/Server.json" ]; then
cat > "$ROOT_DIR/Config/Server.json" << 'EOF'
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

if [ ! -f "$ROOT_DIR/Config/Projects.json" ]; then
cat > "$ROOT_DIR/Config/Projects.json" << 'EOF'
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
# 5. Python Virtual Environment
# ---------------------------
echo ""
echo "--- Setting up Python environment ---"

if command -v python3 &>/dev/null; then
    if [ ! -d "$ROOT_DIR/venv" ]; then
        python3 -m venv "$ROOT_DIR/venv"
        echo "  Created Python virtual environment: venv/"
        echo "  Activate with: source venv/bin/activate"
    else
        echo "  Python virtual environment already exists: venv/"
    fi
else
    echo "  [!!] Python3 not found — skipping virtual environment setup"
fi

# ---------------------------
# 6. Summary
# ---------------------------
echo ""
echo "=== Bootstrap Complete ==="
echo ""
echo "Next steps:"
echo "  1. Review ROADMAP.md for the full implementation plan"
echo "  2. Start with Phase 0 tasks (see ROADMAP.md)"
echo "  3. Build: mkdir -p Builds/Debug && cd Builds/Debug && cmake ../.. && cmake --build ."
echo ""
echo "Optional:"
echo "  - Install Ollama for local AI: curl -fsSL https://ollama.ai/install.sh | sh"
echo "  - Pull a coding model: ollama pull deepseek-coder"
echo "  - Activate Python env: source venv/bin/activate"
echo ""
