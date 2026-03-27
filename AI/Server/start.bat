@echo off
REM start.bat — Launch the Atlas AI Server on Windows
REM Usage:  start.bat [--port 8080] [--model codellama]

setlocal
set "SCRIPT_DIR=%~dp0"
set "REPO_ROOT=%SCRIPT_DIR%..\..\"

echo ==============================================
echo   Atlas AI Server -- MasterRepo Assistant
echo   Project: %REPO_ROOT%
echo ==============================================

REM Find Python
where python >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Python not found. Install Python 3.10+ and retry.
    pause
    exit /b 1
)

REM Create venv if missing
if not exist "%SCRIPT_DIR%.venv\" (
    echo [Setup] Creating virtual environment...
    python -m venv "%SCRIPT_DIR%.venv"
)

REM Activate
call "%SCRIPT_DIR%.venv\Scripts\activate.bat"

REM Install deps
pip install -q -r "%SCRIPT_DIR%requirements.txt"

REM Check Ollama
where ollama >nul 2>&1
if errorlevel 1 (
    echo [Warn]  Ollama not found. Start it from: https://ollama.ai
) else (
    echo [Info]  Ollama found. Ensure 'ollama serve' is running in another terminal.
)

echo [Info]  Starting server at http://127.0.0.1:8080
echo [Info]  Open your browser to http://localhost:8080
echo [Info]  Press Ctrl+C to stop.
echo.

cd /d "%SCRIPT_DIR%"
python atlas_ai_server.py --project "%REPO_ROOT%" %*
pause
