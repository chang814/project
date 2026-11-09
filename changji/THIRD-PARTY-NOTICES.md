# 第三方内容与许可

应用自身的 HTML、CSS、JavaScript、桌面壳及项目说明采用 MIT 许可，见 LICENSE。该许可不覆盖第三方词典内容、考试节选、字体和第三方在线服务。

- 字体：DM Serif Display、DM Sans、Manrope、Outfit、Lora；对应 SIL Open Font License 文本保留在 dist/*LICENSE.txt。
- 旧版词库取自 [kajweb/dict](https://github.com/kajweb/dict) 和 KyleBing/english-vocabulary。核对时未在 kajweb/dict 仓库找到明确覆盖词典数据的许可。KyleBing 仓库提供 [BSD-3-Clause](https://github.com/KyleBing/english-vocabulary/blob/master/LICENSE)，但旧版成品没有逐条记录两路来源。因此此副本移除了成品混合词库、原例句、考试节选和相关补丁，仅附原创示例词条和短文。
- 词典读音：联网请求有道词典音频，不随安装包缓存或打包录音。https://dict.youdao.com/
- 缺失的长句译文：用户按 Tab 后按需请求 MyMemory，仅发送当前句子，不发送学习记录；结果在本机缓存。https://mymemory.translated.net/doc/spec.php
- Windows 版使用 Electron；打包版本带有 Electron LICENSE 和 Chromium LICENSES.chromium.html，保留原许可。https://www.electronjs.org/
