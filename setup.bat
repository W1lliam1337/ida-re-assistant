@echo off
setlocal enabledelayedexpansion

:: Set working directory to script location
cd /d "%~dp0"

:: Check and initialize git submodules if needed
if exist ".gitmodules" (
    if not exist "imgui-client\libs\imgui\imgui.cpp" (
        echo [INFO] Initializing git submodules...
        git submodule update --init --recursive
        if errorlevel 1 (
            echo [ERROR] Failed to initialize git submodules!
            echo.
            echo If you cloned without submodules, run:
            echo   git submodule update --init --recursive
            echo.
            pause
            exit /b 1
        )
        echo [SUCCESS] Submodules initialized
        echo.
    )
)

:: Initialize Visual Studio environment if not already done
if "%VSINSTALLDIR%"=="" (
    echo [INFO] Initializing Visual Studio environment...

    :: Try to find and initialize vcvars
    set "VCVARS_FOUND=0"

    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
        echo [INFO] Found VS 2022 Community
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
        set "VCVARS_FOUND=1"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
        echo [INFO] Found VS 2022 Professional
        call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
        set "VCVARS_FOUND=1"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
        echo [INFO] Found VS 2022 Enterprise
        call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
        set "VCVARS_FOUND=1"
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
        echo [INFO] Found VS 2019 Community
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
        set "VCVARS_FOUND=1"
    )

    if !VCVARS_FOUND! equ 0 (
        echo [ERROR] Could not find Visual Studio installation
        echo.
        echo Please ensure Visual Studio 2019+ with C++ Desktop Development is installed.
        echo Required components:
        echo   - MSVC v142/v143 C++ build tools
        echo   - Windows SDK
        echo.
        echo Download from: https://visualstudio.microsoft.com/
        echo.
        pause
        exit /b 1
    )

    echo [SUCCESS] Visual Studio environment initialized
    echo.
)

:menu
cls
echo ========================================
echo IDA RE Assistant - Setup Menu
echo ========================================
echo.
echo What do you want to do?
echo.
echo [1] Build ImGui Client
echo [2] Install IDA Plugin
echo [3] Setup Claude Desktop (MCP stdio)
echo [4] Start Multi-LLM MCP Server (SSE)
echo [5] Check Release Readiness
echo [Q] Quit
echo.
choice /c 12345Q /n /m "Choose option: "

if errorlevel 6 goto :eof
if errorlevel 5 goto check_release
if errorlevel 4 goto start_sse
if errorlevel 3 goto install_claude
if errorlevel 2 goto install_plugin
if errorlevel 1 goto build_client

:build_client
cls
echo ========================================
echo Building ImGui Client
echo ========================================
echo.

:: Verify compiler is available (should be from vcvars)
where cl.exe >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Visual Studio C++ compiler not found in PATH!
    echo.
    echo The vcvars64.bat initialization may have failed.
    echo Please ensure Visual Studio 2019+ with C++ Desktop Development is installed.
    echo.
    echo Required components:
    echo   - MSVC v142/v143 C++ build tools
    echo   - Windows SDK
    echo.
    echo Download from: https://visualstudio.microsoft.com/
    echo.
    pause
    goto menu
)

:: Check for CMake
where cmake.exe >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] CMake not found!
    echo Install from: https://cmake.org/download/
    echo.
    pause
    goto menu
)

cd /d "%~dp0imgui-client"

echo Configuring CMake...
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed!
    pause
    cd /d "%~dp0"
    goto menu
)

echo.
echo Building Release binary...
cmake --build build --config Release

if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    pause
    cd /d "%~dp0"
    goto menu
)

cd /d "%~dp0"

echo.
echo [SUCCESS] Build complete!
echo Binary: imgui-client\build\bin\Release\ida_re_assistant.exe
echo.
pause
goto menu

:install_plugin
cls
echo ========================================
echo Install IDA Plugin
echo ========================================
echo.
echo Installing ida_mcp_plugin.py to IDA plugins folder...
echo.

set "PLUGINS_DIR=%APPDATA%\Hex-Rays\IDA Pro\plugins"

if not exist "%PLUGINS_DIR%" (
    echo [INFO] Standard plugins directory not found.
    echo Please enter your IDA plugins directory path:
    set /p PLUGINS_DIR="Path: "

    if not exist "!PLUGINS_DIR!" (
        echo [ERROR] Directory does not exist!
        pause
        goto menu
    )
)

echo Target: %PLUGINS_DIR%
echo.

if not exist "%~dp0ida-plugin\ida_mcp_plugin.py" (
    echo [ERROR] Plugin source not found!
    pause
    goto menu
)

copy /Y "%~dp0ida-plugin\ida_mcp_plugin.py" "%PLUGINS_DIR%\" >nul

if %errorlevel% neq 0 (
    echo [ERROR] Failed to copy plugin!
    echo Try running as Administrator
    pause
    goto menu
)

echo [SUCCESS] Plugin installed!
echo.
echo Next: Restart IDA and press Ctrl+Shift+M
echo.
pause
goto menu

:install_claude
cls
echo ========================================
echo Setup Claude Desktop (stdio MCP)
echo ========================================
echo.
echo This configures Claude Desktop to use MCP stdio transport.
echo.
echo NOTE: This is ONLY for Claude Desktop users.
echo For other LLMs, use the standalone client or SSE server.
echo.
pause

python "%~dp0mcp-server\install_mcp.py"

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Installation failed!
    pause
    goto menu
)

echo.
echo [SUCCESS] Claude Desktop configured!
echo.
echo Next steps:
echo   1. Restart Claude Desktop
echo   2. Open IDA and press Ctrl+Shift+M
echo   3. Ask Claude about your binary!
echo.
pause
goto menu

:start_sse
cls
echo ========================================
echo Start Multi-LLM MCP Server (SSE)
echo ========================================
echo.
echo This starts SSE transport for ChatGPT, Gemini, etc.
echo.
echo Checking dependencies...

python -c "import mcp, starlette, uvicorn" 2>nul
if %errorlevel% neq 0 (
    echo [WARN] SSE dependencies not installed!
    echo.
    echo Install with: pip install -r mcp-server\requirements_sse.txt
    echo.
    choice /c YN /m "Install now? (Y/N) "
    if errorlevel 2 goto menu

    pip install -r "%~dp0mcp-server\requirements_sse.txt"
    if %errorlevel% neq 0 (
        echo [ERROR] Installation failed!
        pause
        goto menu
    )
)

echo.
echo Starting SSE server on http://127.0.0.1:13121/sse
echo.
echo Press Ctrl+C to stop the server
echo.

python "%~dp0mcp-server\ida_sse_server.py"

pause
goto menu

:check_release
cls
echo ========================================
echo Release Readiness Check
echo ========================================
echo.

set ERRORS=0

echo [1/5] Client binary...
if exist "imgui-client\build\bin\Release\ida_re_assistant.exe" (
    echo   [OK]
) else (
    echo   [MISSING] Run option 1 to build
    set /a ERRORS+=1
)

echo [2/5] IDA plugin...
if exist "ida-plugin\ida_mcp_plugin.py" (
    echo   [OK]
) else (
    echo   [MISSING]
    set /a ERRORS+=1
)

echo [3/5] MCP servers...
if exist "mcp-server\ida_bridge.py" (
    if exist "mcp-server\ida_sse_server.py" (
        echo   [OK]
    ) else (
        echo   [MISSING SSE server]
        set /a ERRORS+=1
    )
) else (
    echo   [MISSING stdio bridge]
    set /a ERRORS+=1
)

echo [4/5] Documentation...
if exist "README.md" (
    if exist "QUICK_START.md" (
        echo   [OK]
    ) else (
        echo   [MISSING QUICK_START.md]
        set /a ERRORS+=1
    )
) else (
    echo   [MISSING README.md]
    set /a ERRORS+=1
)

echo [5/5] Scripts...
if exist "mcp-server\install_mcp.py" (
    echo   [OK]
) else (
    echo   [MISSING]
    set /a ERRORS+=1
)

echo.
if %ERRORS% EQU 0 (
    echo [SUCCESS] Ready for release!
) else (
    echo [FAILED] %ERRORS% error(s) found
)

echo.
pause
goto menu
