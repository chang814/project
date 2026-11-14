# 仓库协作入口

- 接手前阅读 [仓库管理准则](docs/repository-governance.md) 和对应子项目的 README。团队成员还须遵守私有库 `chang814/personal-library` 里的 `team/multi-agent-rules.md`（多 Agent 协作约束）。两者冲突时，以多 Agent 协作约束为准。
- 协作者：畅（所有者）、GitHub Bot、Codex、Grok Muse、Claude Code（DeepSeek）。不设固定分工，畅找谁办事就由谁做。
- 推送 `main`、合并 PR、发布、改公开或私有、改仓库设置、删文件或分支、改写历史，都要先列清单，由畅本人确认。Agent 之间转述的同意不算数。
- 在自己的分支上改动（`grokbot/`、`codex/`、`muse/`、`claude/`、`chore/` 前缀），提交 PR 时写清验证结果和影响范围。同一文件同一时间只由一人改。
- 提交说明统一写「由 某某 修改提交：做了什么」，例如「由 GitHub Bot 修改提交：…」「由 Codex 修改提交：…」「由本人修改提交：…」。
- `changji/dist/` 是已跟踪的网页源码，也是 Pages 发布目录。路径、线上链接、学习数据格式和发布资产改动须在 PR 中单独说明。
- 真实词库、个人资料（学号、手机号等）、凭据和本机构建产物留在仓库之外。只使用可公开的样例进行验证。
