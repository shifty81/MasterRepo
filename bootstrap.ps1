# ==============================================================================
# MasterRepo Bootstrap Script (Windows PowerShell)
# ==============================================================================
# Purpose:
#   - Verify required tools are installed
#   - Create project directory structure
#   - Set up Python virtual environment for AI/tooling
#   - Initialize default configuration files
#   - Prepare the workspace for development
# ==============================================================================

$ErrorActionPreference = "Stop"

Write-Host "=== MasterRepo Bootstrap ===" -ForegroundColor Cyan
Write-Host "Detected OS: Windows"

$ROOT_DIR = Split-Path -Parent $MyInvocation.MyCommand.Path
Write-Host "Project root: $ROOT_DIR"

# ---------------------------
# 1. Check Required Tools
# ---------------------------
Write-Host ""
Write-Host "--- Checking required tools ---" -ForegroundColor Yellow

function Check-Tool {
    param([string]$Name, [string]$Description)
    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if ($cmd) {
        Write-Host "  [OK] $Name found: $($cmd.Source)" -ForegroundColor Green
    } else {
        Write-Host "  [!!] $Name not found - $Description" -ForegroundColor Red
    }
}

Check-Tool "git" "Required for version control"
Check-Tool "cmake" "Required for building the engine (install from https://cmake.org)"
Check-Tool "python" "Required for AI tools and scripting"

Write-Host ""
Write-Host "--- Checking optional tools ---" -ForegroundColor Yellow
Check-Tool "node" "Optional: needed for Monaco IDE integration"
Check-Tool "npm" "Optional: needed for Monaco IDE integration"
Check-Tool "blender" "Optional: needed for Blender addon pipeline"
Check-Tool "ollama" "Optional: needed for local LLM support"

# ---------------------------
# 2. Create Directory Structure
# ---------------------------
Write-Host ""
Write-Host "--- Creating directory structure ---" -ForegroundColor Yellow

$directories = @(
    "Engine\Render", "Engine\Physics", "Engine\Audio", "Engine\Input",
    "Engine\Window", "Engine\Platform", "Engine\Math",
    "Core\Reflection", "Core\Metadata", "Core\Serialization", "Core\Messaging",
    "Core\Events", "Core\TaskSystem", "Core\CommandSystem", "Core\ResourceManager",
    "Core\ArchiveSystem", "Core\VersionSystem", "Core\PluginSystem",
    "Editor\Core", "Editor\UI", "Editor\Docking", "Editor\Panels",
    "Editor\Viewport", "Editor\Gizmos", "Editor\Overlay", "Editor\Modes",
    "Editor\NodeEditors", "Editor\BuilderEditor", "Editor\PCGEditor",
    "Editor\MaterialEditor", "Editor\Commands",
    "Runtime\World", "Runtime\ECS", "Runtime\Components", "Runtime\Systems",
    "Runtime\Gameplay", "Runtime\Player", "Runtime\Inventory", "Runtime\Equipment",
    "Runtime\Crafting", "Runtime\BuilderRuntime", "Runtime\Damage",
    "Runtime\Collision", "Runtime\Spawn", "Runtime\SaveLoad", "Runtime\UI",
    "AI\Models", "AI\Memory", "AI\Embeddings", "AI\Training", "AI\Context",
    "AI\Prompts", "AI\ArchiveLearning", "AI\CodeLearning", "AI\AssetLearning",
    "AI\ErrorLearning", "AI\ModelManager", "AI\AgentScheduler",
    "Agents\CodeAgent", "Agents\FixAgent", "Agents\BuildAgent",
    "Agents\CleanupAgent", "Agents\AssetAgent", "Agents\PCGAgent",
    "Agents\EditorAgent", "Agents\RefactorAgent", "Agents\DebugAgent",
    "Agents\VersionAgent",
    "Builder\Core", "Builder\Parts", "Builder\Modules", "Builder\PhysicsData",
    "Builder\Collision", "Builder\Damage", "Builder\Assembly", "Builder\SnapRules",
    "PCG\Geometry", "PCG\Textures", "PCG\Audio", "PCG\World",
    "PCG\Quests", "PCG\Rules", "PCG\Validation",
    "UI\Widgets", "UI\Layouts", "UI\Themes",
    "IDE\CodeEditor", "IDE\Console", "IDE\AIChat", "IDE\Debugger",
    "Tools\ServerManager", "Tools\AssetTools", "Tools\BuildTools", "Tools\Importer",
    "Projects\NovaForge\Assets", "Projects\NovaForge\Prefabs",
    "Projects\NovaForge\Parts", "Projects\NovaForge\Modules",
    "Projects\NovaForge\Ships", "Projects\NovaForge\Recipes",
    "Projects\NovaForge\Scenes", "Projects\NovaForge\UI",
    "Projects\NovaForge\Config",
    "Archive\NovaForgeOriginal", "Archive\PioneerForge", "Archive\BuilderOld",
    "Archive\EditorOld", "Archive\OldSystems", "Archive\Broken",
    "Archive\Replaced", "Archive\Generated",
    "WorkspaceState",
    "Versions\Frames", "Versions\Snapshots", "Versions\ObjectFrames",
    "Versions\SystemFrames", "Versions\SceneFrames",
    "TrainingData\Code", "TrainingData\Assets", "TrainingData\Errors",
    "TrainingData\Diffs",
    "External\SwissAgent", "External\AtlasForge", "External\NovaForgeOriginal",
    "Plugins\Editor", "Plugins\Runtime", "Plugins\AI", "Plugins\Builder",
    "Experiments\TestSystems", "Experiments\Prototypes", "Experiments\Temp",
    "Builds\Debug", "Builds\Release", "Builds\Editor",
    "Logs\Build", "Logs\Agent", "Logs\AI", "Logs\Server",
    "Config",
    "Scripts\Lua", "Scripts\Python\Blender", "Scripts\Tools",
    "Docs\Architecture", "Docs\API", "Docs\Builder", "Docs\Editor",
    "Docs\AI", "Docs\Runtime", "Docs\Guides"
)

foreach ($dir in $directories) {
    $fullPath = Join-Path $ROOT_DIR $dir
    if (-not (Test-Path $fullPath)) {
        New-Item -ItemType Directory -Path $fullPath -Force | Out-Null
    }
}

Write-Host "  Created $($directories.Count) directories" -ForegroundColor Green

# ---------------------------
# 3. Python Virtual Environment
# ---------------------------
Write-Host ""
Write-Host "--- Setting up Python environment ---" -ForegroundColor Yellow

$pythonCmd = Get-Command "python" -ErrorAction SilentlyContinue
if ($pythonCmd) {
    $venvPath = Join-Path $ROOT_DIR "venv"
    if (-not (Test-Path $venvPath)) {
        & python -m venv $venvPath
        Write-Host "  Created Python virtual environment: venv\" -ForegroundColor Green
        Write-Host "  Activate with: .\venv\Scripts\Activate.ps1"
    } else {
        Write-Host "  Python virtual environment already exists: venv\" -ForegroundColor Green
    }
} else {
    Write-Host "  [!!] Python not found - skipping virtual environment setup" -ForegroundColor Red
}

# ---------------------------
# 4. Summary
# ---------------------------
Write-Host ""
Write-Host "=== Bootstrap Complete ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. Review ROADMAP.md for the full implementation plan"
Write-Host "  2. Start with Phase 0 tasks (see ROADMAP.md)"
Write-Host "  3. Build:"
Write-Host "     mkdir Builds\Debug; cd Builds\Debug"
Write-Host "     cmake ..\.. -G 'Visual Studio 17 2022'"
Write-Host "     cmake --build . --config Debug"
Write-Host ""
Write-Host "Optional:"
Write-Host "  - Install Ollama for local AI: https://ollama.ai/download"
Write-Host "  - Pull a coding model: ollama pull deepseek-coder"
Write-Host "  - Activate Python env: .\venv\Scripts\Activate.ps1"
Write-Host ""
