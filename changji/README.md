# 畅记 Changji

畅记是我为英语词汇和句子练习做的工具，提供网页版和 Windows 桌面版。练习时可以跟着单词打字、听读音拼写，也可以输入长句。项目支持四级、六级和基础词库，仓库随附原创示例，学习记录保存在本机，可在设置中导出和导入。

## 在线使用

[打开畅记网页版](https://chang814.github.io/project/)，使用内置原创示例词条练习。学习记录保存在当前浏览器，可在设置中导出和导入。自带词库的导入方式见下方的本地运行说明。

## 本地运行

需要 Node.js 22 或更高版本。网页无需安装依赖，在项目目录执行：

    npm start

然后访问 [http://127.0.0.1:4176/](http://127.0.0.1:4176/)。静态托管时使用 dist/ 目录。主练习、词库和键音可离线使用。词典读音和按需翻译依赖外部服务。

## 导入自己的词库

准备 UTF-8 文本，每行一个英文单词。可用 Tab 再填一列中文释义和一列英文释义，参照 [vocabulary-example.tsv](vocabulary-example.tsv)。

    npm run import:vocab -- cet6 "D:\词库\我的词库.tsv"
    npm start

打开或刷新本地网页，在设置里选择六级词库。命令里的 cet6 对应六级，cet4 对应四级，basic 对应基础词汇。再次导入同一类别会替换该类别的本地词库。

导入结果保存在被 Git 忽略的 .local-vocabulary/，由本地 Node 服务读取。GitHub Pages 和桌面打包读取 dist/ 的原创示例。上传仓库时也请把原始词库文件留在项目目录外。

## Windows 桌面版

[下载畅记 v1.1.3 Windows x64 免安装版](https://github.com/chang814/project/releases/download/windows-2026-11-09/Changji-v1.1.3-Windows-x64.exe)，双击运行。也可从源码自行构建：

    npm ci
    npm run desktop
    npm run build:win

便携版构建输出到 `release/v1.1/`。

## 项目结构

- dist/ 包含网页源码、原创示例词条、字体和随附许可。
- desktop/ 和 build/ 包含 Electron 入口及打包图标。
- prepare-*.cjs 和 enhance-exam.cjs 是历史词库处理工具。重新生成词库需自行准备原始数据，运行应用无需重新生成。
- qa-*.cjs 是验证脚本。运行 npm run check 执行基础检查，运行 node qa-content-v11.cjs 检查词库数据。部分历史界面检查另需 Playwright 和 Microsoft Edge。
- docs/ 存放[产品调研](docs/product-research.md)和[复习算法计划](docs/review-algorithm-plan.md)。

## 许可与来源

应用代码及原创示例采用 [MIT 许可](LICENSE)。旧版混合词库和考试节选已从此副本移除。授权核对结果见[第三方内容与许可](THIRD-PARTY-NOTICES.md)。
