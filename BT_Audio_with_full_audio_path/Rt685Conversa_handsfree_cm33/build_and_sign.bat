@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM Run this script from Rt685Conversa_handsfree_cm33 project root.
REM Usage:
REM   build_and_sign.bat [Config] [Jobs]
REM   build_and_sign.bat Debug 24
set "PROJ_DIR=%~dp0"
if "%PROJ_DIR:~-1%"=="\" set "PROJ_DIR=%PROJ_DIR:~0,-1%"

set "SCRIPTS_DIR=%PROJ_DIR%\scripts"
set "IMGTOOL=%SCRIPTS_DIR%\imgtool.py"
set "KEY=%SCRIPTS_DIR%\sign-rsa2048-priv.pem"
set "CONFIG=%~1"
set "JOBS=%~2"
if "%CONFIG%"=="" set "CONFIG=Debug"
if "%JOBS%"=="" set "JOBS=24"
set "BUILD_DIR=%PROJ_DIR%\%CONFIG%"
set "MCUX_IDE_ROOT=D:\NXP\MCUXpressoIDE_25.6.136\ide"
set "MAKE_EXE=%MCUX_IDE_ROOT%\buildtools\bin\make.exe"
set "OBJCOPY_EXE=%MCUX_IDE_ROOT%\tools\bin\arm-none-eabi-objcopy.exe"
set "MAKE_CMD=%MAKE_EXE%"
set "OBJCOPY_CMD=%OBJCOPY_EXE%"
set "CMD_SHELL=%SystemRoot%\System32\cmd.exe"
set "TOOLCHAIN_BIN=%MCUX_IDE_ROOT%\tools\bin"
set "BUILDTOOLS_BIN=%MCUX_IDE_ROOT%\buildtools\bin"

REM MCUboot image parameters (16MB flash, dual-slot layout)
set "ALIGN=4"
set "HEADER_SIZE=0x400"
set "SLOT_SIZE=0x7E0000"
set "MAX_SECTORS=800"
set "VERSION=2.1"

if not exist "%IMGTOOL%" (
    echo [ERROR] imgtool not found: "%IMGTOOL%"
    exit /b 1
)

if not exist "%KEY%" (
    echo [ERROR] signing key not found: "%KEY%"
    exit /b 1
)

if not exist "%BUILD_DIR%" (
    echo [ERROR] build dir not found: "%BUILD_DIR%"
    exit /b 1
)

if not exist "%MAKE_CMD%" (
    echo [ERROR] make not found.
    echo         Expected: "%MAKE_EXE%"
    exit /b 1
)

if not exist "%OBJCOPY_CMD%" (
    echo [ERROR] arm-none-eabi-objcopy not found.
    echo         Expected: "%OBJCOPY_EXE%"
    exit /b 1
)

if not exist "%TOOLCHAIN_BIN%\arm-none-eabi-gcc.exe" (
    echo [ERROR] arm-none-eabi-gcc not found.
    echo         Expected: "%TOOLCHAIN_BIN%\arm-none-eabi-gcc.exe"
    exit /b 1
)

echo [INFO] Building "%CONFIG%" ...
REM Force GNU make to use Windows cmd shell (avoid Git Bash SHELL=/usr/bin/sh issues).
set "SHELL=%CMD_SHELL%"
set "MAKESHELL=%CMD_SHELL%"
set "COMSPEC=%CMD_SHELL%"
set "PATH=%TOOLCHAIN_BIN%;%BUILDTOOLS_BIN%;%PATH%"
"%MAKE_CMD%" SHELL="%CMD_SHELL%" MAKESHELL="%CMD_SHELL%" COMSPEC="%CMD_SHELL%" -C "%BUILD_DIR%" -r -j%JOBS% all
if errorlevel 1 (
    echo [FAIL] build failed.
    exit /b 1
)

set "AXF="
for %%F in ("%BUILD_DIR%\*.axf") do (
    set "AXF=%%~fF"
    set "AXF_NAME=%%~nF"
    goto :axf_found
)
:axf_found
if "%AXF%"=="" (
    echo [ERROR] no .axf found in "%BUILD_DIR%"
    exit /b 1
)

set "IN_BIN=%BUILD_DIR%\%AXF_NAME%.bin"
echo [INFO] Converting AXF to BIN...
echo        AXF: "%AXF%"
echo        BIN: "%IN_BIN%"
"%OBJCOPY_CMD%" -O binary "%AXF%" "%IN_BIN%"
if errorlevel 1 (
    echo [FAIL] objcopy failed.
    exit /b 1
)

set "OUT_DIR=%PROJ_DIR%\scripts\output"
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"
set "OUT_BIN=%OUT_DIR%\%AXF_NAME%_signed.bin"

echo [INFO] Signing image...
echo        IN : "%IN_BIN%"
echo        OUT: "%OUT_BIN%"

python "%IMGTOOL%" sign --key "%KEY%" --align %ALIGN% --header-size %HEADER_SIZE% --pad-header --slot-size %SLOT_SIZE% --max-sectors %MAX_SECTORS% --version %VERSION% "%IN_BIN%" "%OUT_BIN%" --pad --confirm
if errorlevel 1 (
    echo [FAIL] sign failed.
    exit /b 1
)

echo [OK] Signed image generated:
echo      "%OUT_BIN%"
exit /b 0

