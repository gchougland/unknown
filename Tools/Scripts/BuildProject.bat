@echo off
REM Build script for Unknown project using Unreal Build Tool
REM Usage: BuildProject.bat [Configuration] [Platform] [-Force] [-LiveCoding]
REM Default: Development Win64
REM -Force: Attempts to build even if Live Coding is active (may still fail)
REM -LiveCoding: Attempts to trigger Live Coding compilation and show results

setlocal enabledelayedexpansion

REM Set default values
set CONFIGURATION=Development
set PLATFORM=Win64
set TARGET=UnknownEditor
set FORCE_BUILD=0
set USE_LIVE_CODING=0

REM Parse command line arguments
set ARG_COUNT=0
:parse_args
if "%~1"=="" goto args_done
set /a ARG_COUNT+=1

if /i "%~1"=="-Force" (
    set FORCE_BUILD=1
    shift
    goto parse_args
)

if /i "%~1"=="-LiveCoding" (
    set USE_LIVE_CODING=1
    shift
    goto parse_args
)

if %ARG_COUNT% EQU 1 (
    set CONFIGURATION=%~1
)
if %ARG_COUNT% EQU 2 (
    set PLATFORM=%~1
)

shift
goto parse_args
:args_done

REM Get the script directory and project root
set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..\..\Project\Unknown
set UPROJECT_FILE=%PROJECT_ROOT%\Unknown.uproject

REM Check if .uproject file exists
if not exist "%UPROJECT_FILE%" (
    echo Error: Project file not found at %UPROJECT_FILE%
    exit /b 1
)

REM Set Unreal Engine path (UE 5.6)
set ENGINE_PATH=C:\Program Files\Epic Games\UE_5.6\Engine
set BUILD_BATCH=%ENGINE_PATH%\Build\BatchFiles\Build.bat

REM Check if Unreal Engine is installed
if not exist "%BUILD_BATCH%" (
    echo Error: Unreal Engine 5.6 not found at %ENGINE_PATH%
    echo Please update the ENGINE_PATH in this script if your engine is installed elsewhere.
    exit /b 1
)

echo ========================================
echo Building Unknown Project
echo ========================================
echo Configuration: %CONFIGURATION%
echo Platform: %PLATFORM%
echo Target: %TARGET%
echo Project: %UPROJECT_FILE%
if %FORCE_BUILD%==1 echo Force build: Enabled (will attempt despite Live Coding)
echo ========================================
echo.

REM Build the project and capture output
set OUTPUT_LOG=%TEMP%\UBT_Output_%RANDOM%.log
if %FORCE_BUILD%==1 (
    call "%BUILD_BATCH%" %TARGET% %PLATFORM% %CONFIGURATION% "%UPROJECT_FILE%" -WaitMutex -NoHotReloadFromIDE > "%OUTPUT_LOG%" 2>&1
) else (
    call "%BUILD_BATCH%" %TARGET% %PLATFORM% %CONFIGURATION% "%UPROJECT_FILE%" -WaitMutex > "%OUTPUT_LOG%" 2>&1
)

REM Display the build output
type "%OUTPUT_LOG%"

set BUILD_RESULT=%ERRORLEVEL%

REM Check if Live Coding error occurred
findstr /C:"Live Coding is active" "%OUTPUT_LOG%" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo Live Coding is active - Checking compilation status...
    echo ========================================
    echo.
    echo Checking for Live Coding compilation results in log file...
    echo.
    
    REM Check the Unreal log file for compilation results
    set UNREAL_LOG=%PROJECT_ROOT%\Saved\Logs\Unknown.log
    if exist "%UNREAL_LOG%" (
        echo Recent compilation messages from Live Coding:
        echo ----------------------------------------
        REM Use PowerShell to get last 100 lines and filter for compilation-related messages
        powershell -Command "$content = Get-Content '%UNREAL_LOG%' -Tail 100; $matches = $content | Select-String -Pattern 'Compile|Error|Warning|Live.?Coding|Succeed|Failed|Fatal' -Context 0,2; if ($matches) { $matches | ForEach-Object { Write-Host $_.Line; if ($_.Context.PostContext) { Write-Host $_.Context.PostContext[0] } } } else { Write-Host 'No recent compilation messages found. Trigger Live Coding compilation (Ctrl+Alt+F11) and check again.' }" 2>nul
        echo ----------------------------------------
        echo.
        echo Full log available at: %UNREAL_LOG%
    ) else (
        echo Log file not found at: %UNREAL_LOG%
        echo Please trigger Live Coding compilation (Ctrl+Alt+F11) in the editor, then check the output log.
    )
    
    echo.
    echo To see Live Coding compilation results:
    echo   1. Press Ctrl+Alt+F11 in the Unreal Editor to trigger compilation
    echo   2. Run this script again to check the log for errors
    echo   3. Or check the editor's Output Log window directly
    echo.
    
    del "%OUTPUT_LOG%" >nul 2>&1
    exit /b 2
)

REM Check build result
if %BUILD_RESULT% EQU 0 (
    echo.
    echo ========================================
    echo Build succeeded!
    echo ========================================
    del "%OUTPUT_LOG%" >nul 2>&1
    exit /b 0
) else (
    echo.
    echo ========================================
    echo Build failed with error code %BUILD_RESULT%
    echo ========================================
    echo.
    echo Full build output saved to: %OUTPUT_LOG%
    echo.
    
    del "%OUTPUT_LOG%" >nul 2>&1
    exit /b %BUILD_RESULT%
)

