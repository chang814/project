# 五子棋对战游戏

这是我们在《程序设计训练》课程中完成的团队实践作业。项目把五子棋规则、电脑对手和局域网对战整合到一个 Windows 游戏中。

## 主要功能

- 支持本地双人、人机和局域网对战。
- 人机模式有六档难度，较高难度使用 Alpha-Beta 剪枝、迭代加深和连续冲四搜索。
- 支持悔棋、存档、读档、复盘和棋谱导出。

## 编译运行

在 Windows 上准备 TDM-GCC 4.9.2 x64（Dev-C++ 5.11 随附的 MinGW64）和 EGE 20.08，确保 EGE 安装包含 build_ege.bat 使用的 lib\devcpp\graphics64 库。在本目录的 PowerShell 中设置 MINGW_HOME 和 EGE_HOME，再运行脚本。

    $env:MINGW_HOME = 'C:\path\to\MinGW64'
    $env:EGE_HOME = 'C:\path\to\EGE20.08'
    .\build.bat
    .\gobang.exe

将上面的路径替换为自己的安装目录。

## 目录

- src/ 包含棋盘、存档、AI、联网、界面和输入模块。
- tests/ 包含棋盘与存档测试、AI 测试、输入测试以及自对弈工具。
- build.bat 和 build_ege.bat 用于命令行构建。gobang.dev 是原有的 Dev-C++ 工程文件。

## 贡献

这是团队协作成果。我的主要工作是棋局逻辑、存档和 AI 引擎，并参与了联网模块。界面和交互由队友负责。
