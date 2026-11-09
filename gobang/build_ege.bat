@echo off
chcp 65001 >nul
rem ============================================================================
rem  Gobang build script (EGE 20.08 + MinGW-w64)  -  UTF-8 编码
rem  编译器路径和 EGE 路径支持环境变量覆盖：
rem    MINGW_HOME  MinGW-w64 安装目录（含 bin\gcc.exe）
rem    EGE_HOME    EGE 20.08 根目录（含 include\ 和 lib\mingw64\）
rem  未设置时使用下方默认值。
rem ============================================================================
setlocal

if "%MINGW_HOME%"=="" set "MINGW_HOME=C:\Program Files (x86)\Dev-Cpp\MinGW64"
if "%EGE_HOME%"==""   set "EGE_HOME=F:\2026暑期小学期2\工具 材料\ege20.08_all\EGE20.08"

for %%I in ("%MINGW_HOME%") do set "MINGW_HOME=%%~sI"
for %%I in ("%EGE_HOME%")   do set "EGE_HOME=%%~sI"

set "CC=%MINGW_HOME%\bin\gcc.exe"
set "CXX=%MINGW_HOME%\bin\g++.exe"
set "EGE=%EGE_HOME%"

if not exist "%CC%" (
    echo [ERROR] 编译器不存在: %CC%
    echo         请设置环境变量 MINGW_HOME 指向 MinGW-w64 安装目录。
    exit /b 1
)
if not exist "%EGE%\include\graphics.h" (
    echo [ERROR] EGE 头文件不存在: %EGE%\include\graphics.h
    echo         请设置环境变量 EGE_HOME 指向 EGE 20.08 根目录。
    exit /b 1
)

set "CHARSET=-finput-charset=UTF-8 -fexec-charset=GBK"
set "CFLAGS=-O2 -Wall -Wextra %CHARSET%"

cd /d "%~dp0"
if not exist obj mkdir obj

echo [1/12] compile board.c
"%CC%"  -c src\board.c  -o obj\board.o  -Isrc %CFLAGS% || goto :failed
echo [2/12] compile common.c
"%CC%"  -c src\common.c -o obj\common.o -Isrc %CFLAGS% || goto :failed
echo [3/12] compile record.c
"%CC%"  -c src\record.c -o obj\record.o -Isrc %CFLAGS% || goto :failed
echo [4/12] compile ai.c
"%CC%"  -c src\ai.c     -o obj\ai.o     -Isrc %CFLAGS% || goto :failed
echo [5/12] compile ai_eval.c
"%CC%"  -c src\ai_eval.c -o obj\ai_eval.o -Isrc %CFLAGS% || goto :failed
echo [6/12] compile ai_search.c
"%CC%"  -c src\ai_search.c -o obj\ai_search.o -Isrc %CFLAGS% || goto :failed
echo [7/12] compile ai_vcf.c
"%CC%"  -c src\ai_vcf.c  -o obj\ai_vcf.o  -Isrc %CFLAGS% || goto :failed
echo [8/12] compile ai_formation.c
"%CC%"  -c src\ai_formation.c -o obj\ai_formation.o -Isrc %CFLAGS% || goto :failed
echo [9/12] compile net.c
"%CC%"  -c src\net.c    -o obj\net.o    -Isrc %CFLAGS% || goto :failed
echo [10/12] compile ui.cpp
"%CXX%" -c src\ui.cpp   -o obj\ui.o     -Isrc "-I%EGE%\include" %CFLAGS% || goto :failed
echo [11/12] compile input.cpp
"%CXX%" -c src\input.cpp -o obj\input.o -Isrc "-I%EGE%\include" %CFLAGS% || goto :failed
echo [12/12] compile main.cpp
"%CXX%" -c src\main.cpp -o obj\main.o   -Isrc "-I%EGE%\include" %CFLAGS% || goto :failed
echo [LINK] link gobang.exe
"%CXX%" -o gobang.exe obj\board.o obj\common.o obj\record.o obj\ai.o obj\ai_eval.o obj\ai_search.o obj\ai_vcf.o obj\ai_formation.o obj\net.o obj\ui.o obj\input.o obj\main.o "-L%EGE%\lib\devcpp" -lgraphics64 -lgdi32 -limm32 -lmsimg32 -lole32 -loleaut32 -lwinmm -luuid -lgdiplus -lws2_32 -mwindows -static || goto :failed

echo.
echo Build OK: %CD%\gobang.exe
goto :eof

:failed
echo.
echo Build FAILED. See the error messages above.
exit /b 1