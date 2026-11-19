# 第三方内容与许可

应用自身的 HTML、CSS、JavaScript、桌面壳及项目说明采用 MIT 许可，见 LICENSE。该许可不覆盖第三方词典内容、考试节选、字体和第三方在线服务。

- 字体：DM Serif Display、DM Sans、Manrope、Outfit、Lora；对应 SIL Open Font License 文本保留在 dist/*LICENSE.txt。
- 四级、六级词库（`dist/words4.js`、`dist/words.js`）转换自 [KyleBing/english-vocabulary](https://github.com/KyleBing/english-vocabulary)（提交 731240a，`full_line_jsonl/sentence/正序`），采用 BSD-3-Clause 许可，Copyright (c) 2022-2026, KyleBing，许可全文见 `dist/KYLEBING-LICENSE.txt`。保留词条、音标、中文释义、至多 5 个短语和 3 个例句，去掉多词词条与重复项；未收录考试真题例句。
- 来源说明：KyleBing 仓库自述其数据源自 [kajweb/dict](https://github.com/kajweb/dict)（kajweb/dict → KyleBing/dict → 简化）。kajweb/dict 未见明确的数据许可；BSD-3 是 KyleBing 对其整理成果给出的许可，不能代表上游数据的权利状况。若权利人提出异议，将撤下相关词库。
- 基础词汇（`dist/words-basic.js`）仍为原创示例词条。
- Windows 版使用 Electron；打包版本带有 Electron LICENSE 和 Chromium LICENSES.chromium.html，保留原许可。https://www.electronjs.org/
