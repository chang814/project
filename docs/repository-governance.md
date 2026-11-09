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

- 新仓库和新目录采用小写英文短横线命名，例如 `cpp-competitive-programming`、`java-oop-labs`、`lab-01-introduction-to-oop`。语言或构建工具要求的路径、入口和文件名按其约定保留。
- 根目录保留 README、项目目录和公共管理入口。共享文档放 `docs/`，GitHub 模板与工作流放 `.github/`，测试和脚本按项目归属存放。
- 当前作品集的 `gobang/` 和 `changji/` 已符合命名准则。`changji/dist/` 是当前直接维护的静态网页源码和 Pages 发布目录，保留跟踪。安装包和便携版放 GitHub Releases。
- 新库默认分支用 `main`。任务分支用 `codex/<task>`、`grokbot/<task>` 或 `chore/<task>` 等小写名称。新发布用项目名、版本和平台标明资产，例如 `changji-v1.1.3-windows-x64.exe`。现有发布标签和下载链接保留。
- 历史改名先盘点 Pages、README、Actions、Release、外部链接和本地克隆。目录只改大小写时使用临时名做中转，检查 Windows 上的 Git diff。仓库改名后更新引用并验证访问与回退路径。[GitHub 仓库改名说明](https://docs.github.com/en/repositories/creating-and-managing-repositories/renaming-a-repository)

当前 `project` 已有 `https://chang814.github.io/project/` 入口。若采用更清晰的 `portfolio-projects` 仓库名，先决定 Pages 旧入口的迁移方式，再处理主页、README、下载链接和已有用户的浏览器学习记录。Pages 项目站 URL 需要单独迁移验证。

## 日常维护

1. 用 Issue 记录目标、负责人、涉及路径和验收方式。同一范围只由一个执行者认领。Codex 与 GrokBot 在不同分支工作。
2. 提 PR，填写变更范围、实际验证和回滚方式。纯移动或改名与功能变更分开。碰到同一文件的并行任务时，由负责人先协调。
3. 由维护者检查 diff 和 CI，再合并。部署、发布、可见性、分支规则、仓库改名与删除由仓库所有者决定。若启用 ruleset，先选在所有目标 PR 上都会运行的检查。当前 Changji 工作流有路径过滤，文档 PR 使用对应验证，不直接把它设为全库必过项。[GitHub Rulesets](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-rulesets/creating-rulesets-for-a-repository)
4. 每月复核一次仓库索引、在线入口和下载链接、未关闭任务、自动化权限与依赖告警。发布时检查版本、下载文件和校验值。维护中发现的问题写入 Issue，便于下次接手。

## GrokBot 试点

先由所有者确认实际使用的 GrokBot 或 GitHub App 身份。若使用 xAI 的 GitHub App，可核对 [Grok (by xAI) 应用页](https://github.com/apps/grok-by-xai)。安装时核对实际权限和可选仓库。只对作品集试点，私有学习库、资料库与备份库留在试点之外。

- 第一阶段使用只读权限读取代码、Issue 和 PR。若产品只能申请超出试点范围的权限，先用人工提供的公开 PR 链接和摘要验证流程。
- 第一次任务选择只改文档的 PR。让 GrokBot 检查命名、断链、权限变化和验证记录，并返回问题位置、复现方法和建议。维护者确认结果，再评估是否需要评论权限。
- 若后续允许机器人提交代码，限制在任务分支和 PR。保持默认分支、部署、仓库设置与密钥由所有者控制。GitHub App 权限按实际使用的接口核对。[GitHub App 权限说明](https://docs.github.com/en/rest/authentication/permissions-required-for-github-apps)

交接信息统一写在 Issue 或 PR 中，包含仓库、任务链接、基准提交、分支、改动范围、实际验证、待确认问题和下一步。示例提示：

> 请按本准则审阅指定 PR。只核对目录边界、命名、入口链接和验证记录。按影响列出文件、问题与核对方法。由维护者处理改动和合并。
