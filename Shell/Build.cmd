@echo off

set  TEST=%1
if not "%TEST:~0,1%" == "/" (
  shift
)

echo %PATH% | findstr /C:"Embed\Utility" 1>nul
if "%ERRORLEVEL%" == "0" goto Skip
set PATH=%PATH%;D:\Embed\Utility;D:\Embed\Yagarto\Bin
set STMTALK=python ../Host/Script/StmTalk.py

:SKIP

if "%USBADDR%"=="" (
  rem set  USBADDR=.@
  set USBADR=.
)

if "%2" == ""  (
  set STM32_PINS=48
) else (
  set STM32_PINS=%2
)
if "%1" == "/clean"    goto clean
if "%1" == "/run"      goto run
if "%1" == "/burn"     goto burn
if "%1" == "/burnr"    goto burnr
if "%1" == "/burnall"  goto burnall
if "%1" == "/gdb"      goto gdb
if "%1" == "/both"     goto both
if "%1" == "/build"    goto build
if not "%1" == ""      goto error

:build
make
goto end

:gdb
make gdb
goto end

:burn
make burn
goto end

:burnr
make burnr
goto end

:burnall
make burnall
goto end

:run
make run
goto end

:clean
make clean
goto end

:both
make both
goto end

:error
echo Unknown build targets !
echo.

:end
