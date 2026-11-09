# 畅记读音资源

## 当前默认

有道词典英语读音接口，依据当前词库源项目 kajweb/dict 的发音请求文档，type 1 为英音，type 2 为美音。按需预载最多六个词的录音，不批量打包下载。浏览器验证 action 的音频完整播放，时长约 1.34 秒。

来源：https://github.com/kajweb/dict#readme

设备语音保留为手动备选；录音失败时不会静默回退到设备合成。

## 其他已研究方案

- Merriam-Webster 词典音频：https://www.dictionaryapi.com/ ，需申请 key。
- Cambridge 数据授权：https://dictionary.cambridge.org/license.html 。
- Azure 神经语音：https://learn.microsoft.com/azure/cognitive-services/speech-service/text-to-speech ，需要云资源与凭据。
- Free Dictionary API：https://dictionaryapi.dev/ ，本地 API 请求返回 403，未选作默认。
