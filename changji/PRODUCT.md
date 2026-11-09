# 畅记
## Platform
Web, standalone from ChangMind.
## User and purpose
The user is preparing for CET-6 and wants a beautiful, exceptionally smooth typing-based vocabulary practice website.
## Confirmed workflows
Follow typing; listen-and-type using dictionary recordings; continuous sentence/article typing. The last two replace the earlier definition dictation and cloze modes at the user's request. Imported articles remain local and retain per-word checkpoints.
## Implementation choices
Buildless HTML/CSS/JavaScript; browser-local persistence and downloadable backups. User explicitly requested a dark, larger, spacious, minimal interface with richer meanings, visible English definitions, and adjustable word transitions.
## Content
3991 unique entries merged from kajweb/dict CET6_2 + CET6_3 and KyleBing CET6 vocabulary. 3412 have English definitions, 2022 have sentence cloze content. Each mode uses its eligible subset. No assertion of official or complete exam coverage.

Latest explicit direction: iOS-inspired dark UI, clear pending/correct/error letter colors, richer meanings and expanded vocabulary. Stable letter elements and an independent caret improve continuity; typing a letter after successful completion starts the next word when auto advance is on.

CET4 added: 4544 terms, 3737 English definitions, 2712 cloze prompts. Course selection is persisted. Chinese precedes English. No motivational captions or transition entrance animation.
