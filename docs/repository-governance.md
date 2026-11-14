# GitHub 仓库管理准则

## 仓库边界

| 类别 | 放什么 | 公开范围 |
| --- | --- | --- |
| 个人主页 | 自我介绍、精选项目、公开入口 | 公开。主页仓库保留账号同名 |
| 作品集 | 可以独立运行或演示的项目、运行说明、来源与贡献 | 公开前核对源码和素材授权 |
| 学习与实验 | 按语言、课程或题目组织的代码和笔记 | 默认私有 |
| 资料与备份 | 原件、快照、恢复说明 | 私有，按用途分开。凭据和含凭据的配置另存 |

作品集可以保留多个项目。项目需要独立发布周期、权限或部署时，再评估拆库。每个项目目录至少有 README，记录用途、运行方法、验证入口和许可或来源。团队项目保留贡献说明。

## 结构和命名

- 现有仓库沿用已有名称：学习库为 `Java-OOP-Labs` 和 `C-CPP-Competitive-Programming`，不再改成全小写。作品集里的项目目录用小写英文短横线，例如 `gobang/`、`changji/`。语言或构建工具要求的路径、入口和文件名按其约定保留。
- Java 实验按 `Lab-01` 到 `Lab-10` 分目录。C++ 题目目录不用 Problem-编号，写成「英文知识点在前、中文题名在后」，例如 `Maximin-Distance-Placement-奶茶店选址`；已有英文题名的题目，后半段也写成短横线连接，例如 `Stack-Reverse-Words-Text-Reverse`。
- 根目录保留 README、项目目录和公共管理入口。共享文档放 `docs/`，GitHub 模板与工作流放 `.github/`，测试和脚本按项目归属存放。
- 当前作品集的 `gobang/` 和 `changji/` 已符合命名准则。`changji/dist/` 是当前直接维护的静态网页源码和 Pages 发布目录，保留跟踪。安装包和便携版放 GitHub Releases。
- 新库默认分支用 `main`。任务分支按执行者加前缀：`grokbot/<task>`、`codex/<task>`、`muse/<task>`、`claude/<task>`，杂项用 `chore/<task>`。新发布用项目名、版本和平台标明资产，例如 `changji-v1.1.3-windows-x64.exe`。现有发布标签和下载链接保留。
- 历史改名先盘点 Pages、README、Actions、Release、外部链接和本地克隆。目录只改大小写时使用临时名做中转，检查 Windows 上的 Git diff。仓库改名后更新引用并验证访问与回退路径。[GitHub 仓库改名说明](https://docs.github.com/en/repositories/creating-and-managing-repositories/renaming-a-repository)

当前 `project` 已有 `https://chang814.github.io/project/` 入口。若采用更清晰的 `portfolio-projects` 仓库名，先决定 Pages 旧入口的迁移方式，再处理主页、README、下载链接和已有用户的浏览器学习记录。Pages 项目站 URL 需要单独迁移验证。

## 提交署名

每条提交说明都写「由 某某 修改提交：做了什么」，例如「由 GitHub Bot 修改提交：…」「由 Codex 修改提交：…」「由 Muse 修改提交：…」「由 Claude Code（DeepSeek）修改提交：…」「由本人修改提交：…」。提交中不放 token、密码、学号或手机号。

## 日常维护

1. 用 Issue 或 PR 记录目标、执行者、涉及路径和验收方式。同一范围同一时间只由一个执行者改，各 Agent 在各自分支工作。
2. 提 PR，填写变更范围、实际验证和回滚方式。纯移动或改名与功能变更分开。碰到同一文件的并行任务时，先停下告诉畅。
3. 执行者检查 diff 和 CI 后给出改动清单，由畅本人确认再合并。部署、发布、可见性、分支规则、仓库改名与删除都由畅决定。若启用 ruleset，先选在所有目标 PR 上都会运行的检查。当前 Changji 工作流有路径过滤，文档 PR 使用对应验证，不直接把它设为全库必过项。[GitHub Rulesets](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-rulesets/creating-rulesets-for-a-repository)
4. 每月复核一次仓库索引、在线入口和下载链接、未关闭任务、自动化权限与依赖告警。发布时检查版本、下载文件和校验值。维护中发现的问题写入 Issue，便于下次接手。

## 多 Agent 协作

协作者有畅（所有者）、GitHub Bot、Codex、Grok Muse 和 Claude Code（DeepSeek）。不设固定分工，畅找谁办事就由谁做。完整约束见私有库 `chang814/personal-library` 的 `team/multi-agent-rules.md`，与本准则冲突时以它为准。

- 推送 `main`、合并 PR、新建或删除仓库、改公开或私有、改仓库设置、删文件或分支、改写历史，都只有畅能批准。Agent 之间转述的同意不算数。
- 动手前先列清单：改哪个仓库、哪些文件、提交说明。畅确认后再推送。
- 登录、2FA、token 和 SSH key 交给畅处理，不进聊天和提交。默认分支、部署、仓库设置与密钥由畅控制。

交接信息统一写在 Issue 或 PR 中，包含仓库、任务链接、基准提交、分支、改动范围、实际验证、待确认问题和下一步。示例提示：

> 请按本准则审阅指定 PR。只核对目录边界、命名、入口链接和验证记录。按影响列出文件、问题与核对方法。改动清单交畅确认后再合并。
