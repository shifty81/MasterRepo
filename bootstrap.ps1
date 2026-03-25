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

$ROOT_DIR = Split-Path -Parent $MyInvocation.MyCommand.Path

# ── Logging ──────────────────────────────────────────────────────────────────
# Capture ALL bootstrap output to a log file so nothing is lost if the
# console window closes unexpectedly.
$LogDir = Join-Path $ROOT_DIR "Logs\Build"
if (-not (Test-Path $LogDir)) {
    New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
}
$BootTimestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$BootLog = Join-Path $LogDir "bootstrap_$BootTimestamp.log"

# Start transcript — captures everything to the log file
Start-Transcript -Path $BootLog -Append | Out-Null

Write-Host "=== MasterRepo Bootstrap ===" -ForegroundColor Cyan
Write-Host "Detected OS: Windows"

Write-Host "Project root: $ROOT_DIR"
Write-Host "Log file: $BootLog" -ForegroundColor DarkGray

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
# 4. GLFW Dependency
# ---------------------------
Write-Host ""
Write-Host "--- GLFW (required for Editor window) ---" -ForegroundColor Yellow
Write-Host "  CMake will auto-download GLFW 3.4 via FetchContent if it is not"
Write-Host "  found on your system. No manual install is required."
Write-Host "  For faster builds or offline use you can pre-install GLFW via:"
Write-Host "    Option A (vcpkg — recommended):"
Write-Host "      git clone https://github.com/microsoft/vcpkg `$env:USERPROFILE\vcpkg"
Write-Host "      `$env:USERPROFILE\vcpkg\bootstrap-vcpkg.bat"
Write-Host "      `$env:USERPROFILE\vcpkg\vcpkg install glfw3:x64-windows"
Write-Host "      Then pass: -DCMAKE_TOOLCHAIN_FILE=`$env:USERPROFILE\vcpkg\scripts\buildsystems\vcpkg.cmake"
Write-Host "      (Or use C:\vcpkg if you prefer a system-wide install and have admin rights)"
Write-Host "    Option B (manual): download glfw-3.4.bin.WIN64.zip from glfw.org"
Write-Host "      and extract to C:\glfw; pass -DGLFW_LIB=C:\glfw\lib-vc2022\glfw3.lib"
Write-Host ""

# ---------------------------
# 5. Summary
# ---------------------------
Write-Host ""
Write-Host "=== Bootstrap Complete ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Bootstrap log saved to: $BootLog" -ForegroundColor DarkGray
Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. Launch the editor (recommended):"
Write-Host "     cmake --preset windows-x64-debug    # configure (downloads GLFW if needed)"
Write-Host "     cmake --build Builds/windows-x64-debug --config Debug --target AtlasEditor"
Write-Host "     .\Builds\windows-x64-debug\Debug\bin\AtlasEditor.exe"
Write-Host ""
Write-Host "  2. Full build via build_all.sh (Git Bash / MSYS2):"
Write-Host "     bash Scripts/Tools/build_all.sh"
Write-Host ""
Write-Host "  3. Classic CMake workflow:"
Write-Host "     mkdir Builds\Debug"
Write-Host "     cd Builds\Debug"
Write-Host "     cmake ..\.. -G 'Visual Studio 17 2022' -A x64"
Write-Host "     cmake --build . --config Debug --target AtlasEditor"
Write-Host ""
Write-Host "Optional:"
Write-Host "  - Install Ollama for local AI: https://ollama.ai/download"
Write-Host "  - Pull a coding model: ollama pull deepseek-coder"
Write-Host "  - Activate Python env: .\venv\Scripts\Activate.ps1"
Write-Host ""

# Stop transcript logging
try { Stop-Transcript | Out-Null } catch { }
