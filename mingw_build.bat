@echo off
IF EXIST C:\MinGW\bin\ (goto compile) else (goto dieTerribleDeath)

REM
REM Begin Compilation
REM
:compile
setlocal

echo.
echo.
echo [ STARTING COMPILATION ]

set PATH=%PATH%;C:\MinGw;C:\MinGW\bin;C:\MinGW\lib;

C:\MinGW\bin\gcc.exe ^
-g -Wall -Werror -Wextra -Wshadow -Wcast-align -std=c11 ^
-fno-diagnostics-color -Wno-unknown-pragmas ^
engine.c ^
engine_tools\ogl_tools.c ^
win\win_platform.c ^
renderers\triangle_renderer.c ^
data_structures\uDynamicArray.c ^
-Inix -Iwin -Iengine_tools -Irenderers -I. -Idata_structures ^
-lopengl32 -luser32 -lgdi32 -lshell32 -lodbccp32 ^
-o engine

echo [ COMPILATION COMPLETE ]
echo.

endlocal
goto dieHonorableDeath

REM
REM Unalbe to find MinGW
REM
:dieTerribleDeath
echo.
echo.
echo    [ ERROR ] MinGW not found at location C:\MinGW\bin

:dieHonorableDeath
echo.
