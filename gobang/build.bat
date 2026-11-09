@echo off
chcp 65001 >nul
rem ============================================================================
rem  Gobang (five-in-a-row) one-click build script  -  Group 15
rem  完整构建脚本（含 EGE 图形库 + 全部 12 个模块）见 build_ege.bat，
rem  本脚本直接调用它，保持「双击 build.bat 即可编译」的约定。
rem ============================================================================
call "%~dp0build_ege.bat"