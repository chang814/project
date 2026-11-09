# 五子棋对战游戏

《程序设计实践》课程团队项目。支持本地双人、人机和局域网对战，以及悔棋、存档、读档和棋谱导出。人机提供六档难度，搜索策略随难度变化。

## 编译运行

在 Windows 上准备 MinGW-w64 和 EGE 20.08，确保 EGE 安装包含 build_ege.bat 使用的 lib\devcpp\graphics64 库。在本目录的 PowerShell 中设置 MINGW_HOME 和 EGE_HOME，再运行脚本。

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
