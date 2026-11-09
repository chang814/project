---
name: 畅记 CET-6
description: Dark, spacious vocabulary practice focused on one word.
colors:
  background: "#111413"
  surface: "#171b19"
  foreground: "#dbe3de"
  muted: "#a4b0a8"
  accent: "#b7d2c1"
  error: "#e6a29a"
typography:
  display:
    fontFamily: "Manrope, Segoe UI, sans-serif"
    fontSize: "clamp(56px, 7vw, 96px)"
    fontWeight: 600
    lineHeight: 1.05
    letterSpacing: "-0.03em"
  body:
    fontFamily: "DMSans, Segoe UI, Microsoft YaHei, sans-serif"
    fontSize: "16px"
    lineHeight: 1.7
rounded:
  control: "6px"
  dialog: "16px"
spacing:
  gap: "20px"
  section: "40px"
---
# Design System: 畅记
## 当前视觉约束 · v1.1.2 r3 网页预览

本节为当前规范，后文保留历史决策。最终样式以 `dist/refine.css` 为准，加载顺序为 style → ios → refine。Operate 模式保留单词居中、安静留白和三种练习方式，不新增练习页控件。正文使用 DMSans／系统中文；中文释义 16px/1.8、英文 15px/1.7、词性 14px。单词继续使用已选字体及字号。页脚签名不显示，历史 Kai 字体规则不代表当前界面。

### 主题与颜色角色

八种主题的原色集中在 refine.css 顶部，根节点和色板共用一张表。JS 只保存主题标识，浏览器 theme-color 读取 `--canvas`。snow、mist、sand 使用 light color-scheme，其余使用 dark。

| 主题 | --canvas | --accent | --word-entered | --salmon（错误） |
| --- | --- | --- | --- | --- |
| sage | #111413 | #b7d2c1 | #dcece2 | #e6a29a |
| nordic | #151b22 | #b6cddd | #deedf7 | #e8a8a5 |
| warm | #191715 | #d2bea1 | #eee1ce | #e3a199 |
| snow | #eef1ee | #476d58 | #203e2d | #a74338 |
| graphite | #17191c | #c2c8d0 | #edf0f4 | #f1a79f |
| ocean | #102025 | #98c9c5 | #d3eeeb | #eda8a2 |
| mist | #edf1f5 | #446c89 | #294d69 | #a53e3c |
| sand | #f3f0e9 | #74644c | #51432e | #a84238 |

- `--surface / --raised / --selected-surface`：面板、控件、选中背景；`--text / --secondary`：正文和次要信息。
- `--focus-ring` 对应 accent；`--selection-bg / --selection-ink` 对应选中背景与已输入文字。文本选择、光标、焦点环、进度、滚动条、记录状态、朗读和收藏反馈跟随主题。
- `--word-idle / --word-pending / --word-entered`：待输入词、长句待输入字母、正确字母；跟练和长句共用正确输入角色。错误使用 `--danger: var(--salmon)` 并保留下划线。
- `--control-hover / --control-active / --control-disabled`：hover、按下和禁用；禁用保持原背景与文字，opacity 为 .45，禁止交互位移。开关、下拉框、选中项及键盘焦点保持可辨识。
- `--progress-fill / --word-pending / --scrollbar-quiet` 使用每主题明确色值，保留原混色比例，组件不再依赖 color-mix。固定主题色只出现在主题表；蒙版中的黑色仅代表遮罩 alpha。

### 动效与兼容

控件即时反馈 140ms；页签指示、点击回弹、下拉菜单 160ms；设置面板内容、关闭及主题色交接 180ms。连续切换主题清理前次定时器，最终选项持久化。字母碰撞、退格 220ms、整词退场 360ms、阅读区 FLIP、长句位移与进度保持现有节奏。

不使用装饰性 backdrop blur，清理旧页脚 blur。长句邻词 0–6px blur 用于标明输入焦点，仍限定在裁切行内，与边缘 mask 一同保留。prefers-reduced-motion 下停止 CSS/JS 控件动画，输入和切换即时生效；本轮不宣称提升帧率或通过真实 Safari 性能测试。

验证见 `QA-v1.1.2-ui.md`，截图和浏览器报告位于 `outputs/v112-ui/`。网页仍为 v1.1.2；EXE 等待用户检查确认，v1.0 冻结。本轮不改词库、例句、来源核验层级及存储结构。

## 历史设计记录

## Overview
User-directed dark, spacious, minimal practice environment. One dominant word, English explanation beneath, expanded Chinese senses below. Navigation is intentionally quiet and contains only practice, vocabulary, review, and settings.
## Colors
Near-black green background; pale lettering; sage correct-input feedback; salmon errors. Resting word color is #989eab; correct letters are #64d2ff; errors are #ff817b with an underline. Semantic color is limited to input feedback and small controls.
## Typography
Native system sans typography follows iOS when available and Segoe UI Variable on Windows. Desktop words reach 110px at weight 600; English definitions are 21px. Mobile definitions are 18px.
## Layout
1220px maximum main container, 60px desktop horizontal padding, 23px mobile padding. Header navigation is centered. The practice stage has no card boundary or shadow. Chinese meanings preserve distinct parts of speech on separate lines. One subtle rule separates example content.
## Elevation & Depth
Practice surface is flat. Only the settings dialog has an overlay and elevation.
## Shapes
12px segmented mode controls with a sliding selected background, restrained outlined keyboard hints, monochrome SVG icons.
## Components
Current word uses instant per-character feedback. Word transitions animate (220ms entrance), and the independent caret moves in 65ms; reduced-motion removes animation. Successful input retains its input element and offers a visible next-word button. Auto transition delay is adjustable from 400–3500ms or disabled. Settings is reachable on desktop and mobile.
## Do's and Don'ts
Preserve the user's dark, large, spacious brief. Do not reintroduce a sidebar or permanent statistics dashboard. Definitions and examples must come from recorded vocabulary sources; missing content must not be fabricated.

Latest CSS authority: dist/ios.css, loaded after style.css. iOS-inspired native segmented control, rounded capsule navigation, switch controls, and large sans-serif type implement the user request. Word detail disclosure contains full recorded senses, up to five phrases and four examples.

Study-first revision: Chinese meanings precede English definitions. Mode captions and footer shortcuts are hidden; no word entrance animation. Caret moves in 45ms without blinking. Course selector beside brand. Full source text stays available in details.

Word face now uses self-hosted Manrope 600; letter effects use Web Animations with stable inline-block glyphs. Select spring, fade, ripple or none in settings; OS reduced-motion disables effects. Settings includes preview.

Practice layout refinement: compact mode/lesson row, slim session progress, font selector (Manrope, bundled WordSerif, native sans). No keyboard guide. Added lift and snap letter effects, completion underline and short definition continuity. Stable letter nodes and caret retain existing input timing. Reduced motion and no-motion settings suppress new effects.

Adaptive practice: configurable initial group size 5/10/20/30/50, seeded shuffle, weak-first, and due+new ordering. Incorrect/hinted/skipped words repeat after up to three intervening items, capped at two repeats per word/session; correct recalls schedule 1/3/7/14/30-day due dates. Local memory records are included in backup. Font and presentation options live in settings. Optional whole-workspace feedback and independent exit glyphs never delay input. This is a local heuristic, not an AI model.

Definition and tactile polish: show up to five complete source meaning groups without semicolon truncation. Normalize/deduplicate English definitions; never repeat definition as an example fallback; disclosure contains additional English senses only. Six locally hosted/native font choices. Combined preview runs typing, the selected page feedback on a miniature practice surface, then actual letter disappearance using the selected exit effect. Correct and incorrect deletions share exit feedback; ghost glyphs are hidden from accessibility.

Motion correction: independent letter/page/exit switches. Letter effects have distinct keyframes and durations (150–360ms). Auto advance starts visible old-word exit 360ms before switching, with original glyphs hidden to avoid duplicate overlays. Removed uniform completion fade. Cache-versioned JS/CSS entry URLs. Browser verification captured eight distinct computed motion styles and real auto-advance exit with letter effect disabled.

Stable practice geometry: viewport-responsive fixed-height grid assigns word, phonetic, explanation, feedback and actions explicit rows. Chinese/English definitions share a bounded scrollable reading region, reset on each word. Context boundary reserves its position even without an example in follow mode. Desktop reading measure 800px; mobile uses available width with wrapping and stable scrollbar gutter. Verified multiple word switches share document boundary position; mortgage long-definition case scrolls internally at 390px with no horizontal overflow.

Definition visual refinement: muted inline part-of-speech labels, complete Chinese sense rows, separate English sense paragraphs with left alignment for longer content. One 210ms opacity/4px arrival on the content wrapper replaces competing per-block entrances. Wrapper remains inside fixed scroll area. English definition fallback uses aggregate en only when individual definitions are missing, preventing duplicate combined senses.

Quiet interaction revision: remove hint/skip buttons and keyboard triggers, practice success/error prose, success underline and auto-mode next button. Preserve manual next control. Definitions crossfade through an aria-hidden outgoing snapshot (100ms) and delayed new text (220ms), with no movement. Nonessential settings prose removed; sources collapsed. Fixed context boundary unchanged.

Visual refinement: neutral graphite surfaces, quiet text navigation with an active underline, transparent course selector and utility icons, restrained native segmented controls, lighter secondary typography and brighter resting word. Dialog, library and disclosure surfaces share neutral colors and consistent radii. Existing fixed practice geometry, definition crossfade and input effects preserved. Checked desktop and 390px layouts with no horizontal overflow.

Whole-page feedback refined, retained: all four options now use restrained scale envelopes around the word region, no x/y translation. Continuous typing samples the current visual state before restarting the envelope; letter feedback remains independent. Verified live input has a scale-only workspace matrix with zero x/y translation.

Learning records and foundational vocabulary: separate Basic course has 2,114 single-word entries merged from kajweb/dict PEP primary and junior books, enriched by current CET records; 2,026 have English definitions and 1,700 eligible sentence prompts. Records log completed attempts, mode/course, error positions, backspaces and capped active inter-key duration; retain latest 10,000 locally, include in JSON merge backup and CSV export. A quiet records route has date/course/mode filters, first-pass spelling rate, mode performance, daily summaries, difficult words and paged attempt details. Legacy progress remains aggregate only. Discrete mouse wheels ease over RAF; trackpad/touch/zoom stay native. Voice settings expose available system voices; external recorded audio still requires API access.

Focused controls revision: main surface now has brand/settings, practice mode text tabs and word progress only. Course and lesson selection moved into settings. Settings is an accessible three-tab sheet: Study (course/group/review/advance), Feel (font/letter/exit/audio/preview), Data (library/review/records/backups/sources). Secondary routes expose a return-to-practice control. Removed page motion selector; a fixed 160ms 0.06-percent scale envelope accompanies keystrokes, disabled by OS reduced motion. Checked tab switching, management routes, return navigation and 390px overflow.

Changji revision: renamed visible brand/title and backups to 畅记, retained existing storage key for continuity. Removed all whole-page motion calls. Dictation/cloze now have stable one-em answer cells and measured caret geometry; word fit recalculates after viewport/font changes, with masked underline placeholders. Hint/Tab reveals answer and records a lapse. Practice exit glyphs are clipped within their word viewport. Mode/settings tabs use matching sliding active backgrounds; moderate UI font sizes. Default pronunciation streams dictionary recordings from the source-documented Youdao endpoint, with English/US accent switching and device speech as explicit alternative. Browser verified audio finished (1.34s), dictation width stable and cloze completes after hint. Key audio uses warmed interactive AudioContext, a filtered 32ms tactile transient, short attack/release and six-voice cap. At group completion, Enter starts the next group.

Exam-context enrichment: source-marked CET6 realExamSentence records from the two existing source books now take priority for 1,518 terms; excerpts retain wording and show supplied year, paper and question type. Missing translations stay empty. Collocation-rule matches can reorder compatible Chinese senses, with no claim of exhaustive sense-frequency statistics. Retains dictionary examples as fallback and in disclosure.


2026-10-31 Operate refinement: dist/refine.css is the final CSS authority. Preserve one centered word stage; Chinese, English and sentence content share a left aligned 720px reading column. Neutral dark surfaces replace borders and decorative halos. Word feedback uses short, distinct, interruptible transform/opacity motions; correct letters resolve into foreground, caret carries accent, errors use salmon. No page motion. Controls target 44px, with native focus, reduced motion, and readable 17px explanations. Three settings tabs and all study behavior retained.

2026-10-31 focus revision: brand is a document-end footer link; settings and mode selectors occupy the top rail. Follow/dictation word is centered around 42svh with responsive minimum spacing; cloze reserves its sentence row above the same focal area. Favorite is grouped beside pronunciation. Pending word #939ba5, entered letters #d9e9df, muted jade accent #b4d0c2. Surfaces #16181b and #1e2125 sit close to the #111214 canvas. UI labels 14px, reading and settings 16px; behavior and existing font choices remain.

2026-10-31 balance and settings: remove vertical auto margin on main; reduce stage spacing and default visible meanings to two POS groups with three senses each, plus one English definition. All meanings and remaining English senses stay in expanded details, along with examples. Settings uses a fixed-height grid sheet with header and tabs pinned, independently scrolling panels, 200ms entrance / 130ms exit and 170ms pane transition. Inline group picker previews words, retains explicit selection under smart ordering and across resume. Native select controls remain native.

2026-10-31 palette microadjustment: sage-neutral canvas #111413, surfaces #171b19/#202623, foreground #dbe3de, secondary #a4b0a8, accent #b7d2c1. Pending word #a1afa6 and entered #dcece2 carry the primary hierarchy. Focus rings, scrollbar, selected tabs, switches, previews and record semantic colors use this same palette; salmon remains the error exception.

Brand signature now sits at the lower left of the shared content grid at 12px and tertiary contrast, removed from the visual axis.

2026-10-31 details shortcut: while the word input is focused, Tab toggles full definitions and examples with no visible disclosure caption. Unsolved dictation/cloze first uses Tab to reveal the hint; subsequent Tab may disclose details after reveal. Shift+Tab and settings keyboard navigation remain native. Opening details after completion suspends automatic advance until closed.

2026-10-31 motion and proximity: mode/settings rails are transparent with a slim 28px selected surface inside unchanged 44px touch targets. Indicator glides for 300ms with restrained overshoot; clicked label gets a 230ms settling motion and settings content follows horizontal direction. Footer uses native Kai-style serif, slight rotation and 0.3px blur. Scroll thumbs are 6px with transparent inset, no arrows and muted sage hover/drag colors. Word moves down 18-20px. Expanded details use content-height reading rows with a 164px cap, directly followed by full text; word rows remain fixed so opening does not displace the word.

2026-10-31 unified transitions and themes: quick group button at upper left opens study settings with group picker expanded. Root scrollbar gutter remains reserved; root thumb appears during scrolling instead of popping in when details grow. Full details enter over 320ms and exit over 170ms; rapid toggles cancel and reverse safely. Mode explanations use a 340ms FLIP transform plus a short opacity overlap. Settings and backdrop enter for 320ms / exit 200ms; data routes await sheet exit then animate content in. All setting selects use an accessible themed popover with native-select fallback, arrows/Home/End/Enter/Escape, selected mark, viewport-clamped placement and no clipped menus. Four persisted palettes: sage, Nordic blue, warm charcoal, Nordic light. Root semantic colors apply to controls, selected rails, typography, scroll thumbs, record views and pronunciation feedback. Reduced motion disables transitions.

2026-10-31 stability audit: root reserves gutters on both sides; header uses the content containing block rather than 100vw. Header controls stay outside the animated page content, so route transitions cannot stretch or displace them. Library hover and secondary descriptions use theme roles. Initial definitions flow naturally without an inner scroller; long English preview is limited to two lines, with the full text retained in details. Details use compact section/item groups and a two-column phrase list. Settings scroll rail sits 12px farther right with 24px content clearance, low-contrast thumbs and arrow-free WebKit styling. Dropdowns reposition on ancestor scroll/resize, close when their anchor leaves the panel, and contain wheel scrolling. Tab rails are smaller; settings uses a short sliding underline. Data backup copy explains browser-local storage and merge semantics. Data routes fade content out and in with cancelable opacity animations; controls remain interactive and outside the animated region. The adaptive scheduler and ordering functions are unchanged.

Color research: Apple Dark Mode (semantic colors and base/elevated backgrounds), IBM Carbon Color Overview (same roles across themes; layered surfaces), and Material Color Roles informed the state palette audit. Links: https://developer.apple.com/design/human-interface-guidelines/dark-mode ; https://www.carbondesignsystem.com/building-blocks/foundations/color/overview ; https://m3.material.io/styles/color/roles .

## 2026-10-31 interaction refinement
Preserve the existing palette and word-focused composition. Complete definitions retain their original 640px vertical reading column; phrases retain their existing two-column arrangement. Tab toggles a persisted expanded preference across words and reloads. Primary meanings grow naturally without an inner scrollbar. The visible next-word action has its own layout height and a 16px gap before the complete definitions. Space advances only a completed word; Enter remains available. A held Space does not advance repeatedly.
The settings control is fixed to the viewport; settings dropdowns dismiss when their surrounding panel scrolls. Library row backgrounds remain unchanged on hover. Key sound uses a pre-rendered short tactile sample, a volume control and resumed interactive AudioContext. Dictionary playback starts before completion bookkeeping and preloads the following two entries.

Final refinement: next-word arrow and hint icon are positioned alongside pronunciation, outside definition layout. Both provide accessible labels and hover titles; Space and Enter remain available. The initial and expanded reading columns share a centered 560px measure with left-aligned text. Group progress advances immediately on completion through scaleX easing and a single restrained track pulse; reduced motion removes it.

## 2026-10-31 reading and theme follow-up
Operate refinement preserves the existing word display and motion. The reading column uses a 520px measure with a 28px optical shift right only above 760px; mobile uses the available width without an offset. Definitions remain left-aligned and use 16px Chinese / 15px English. The brand returns to upright 12px text at the lower left of the document. Four additional semantic palettes (graphite, ocean, mist, sand) supplement the existing four; three-color swatches appear in the selected control and each menu option. Tab works from the typing field and the definition area, retains expanded or collapsed preference across words and reloads, and guards against a stale close-animation callback. Existing practice modes are retained pending a separate choice of replacements.

## 2026-10-31 focus and continuous input
Collapsed definitions place the word at the viewport center (optically adjusted on mobile). Tab expands reading while moving the word upward through a cancelable 360ms FLIP transition; closing fades the details over 160ms while the word returns to center. Rapid reversals retain the current visual position and persisted preference. Word-to-word definitions use opacity only, with outgoing115ms and incoming280ms delayed90ms. The caret stays between .18 and .42 opacity. Group progress uses an850ms transform transition at low contrast; no pulse or end dot. Resetting a group resets progress immediately.
The visible modes are 跟练, 听音拼写 and 长句输入. Internal mode keys are retained for backward compatibility. Listening hides spelling, phonetic notation and definitions until completion or an explicit word hint, starts dictionary audio and keeps the replay button. Continuous input centers one word on a single moving rail, preserving DOM neighbors while changing their opacity and blur. Edge masks fade to the canvas on both sides; they are functional content fading, not decorative backgrounds. The focused word scales to narrow viewports. Space advances only an accurately typed token; punctuation at word ends is carried through automatically. Resize/font loading preserves current input. Imported articles and per-word checkpoints stay in browser storage. Completed passages have independent history, avoiding changes to vocabulary mastery or adaptive scheduling.

## 2026-10-31 disclosure and feedback follow-up
Long input hides its progress track, live count, and visual completion totals. Tab opens a local word meaning, centered sentence translation, and centered source row; the word meaning row keeps its existing alignment. Known dictionary translations and the local translation cache render without a loading flash. Only the current sentence is sent to MyMemory when no local translation exists, in UTF-8 chunks below500bytes; machine output is labeled and failures expose retry. Completed metrics remain in the history and accessible status.
Each new sentence/material clears its rail nodes and observer target before rendering, while typing within a sentence retains letter and caret nodes. Wrong characters display the actual typed character and use the same animateLetter, exitGlyph, error color, underline and key sound as follow mode. Pending characters remain dimmer without changing glyph metrics; compact words retain secondary-text contrast.
Listening uses equal-width flex cells; a hint reveals spelling only, leaving definitions hidden until completion. Tab uses a coordinated400ms word movement with the details following the same vector. Follow mode moves slightly upward. Palette changes interpolate over420ms. Record filters now share the accessible popover controls, mounted in the active modal or document; they register only after their controls connect. Returning between pages resets document scrolling before the incoming fade.

## Listening shortcut
Tab in unfinished listening practice invokes the same spelling hint as the hint button, including when focus remains on an audio/action control in the stage. It leaves meanings hidden and is idempotent for the current word. Once completed, Tab retains the complete-definition toggle. Shift+Tab and modifier combinations keep their existing behavior.

## Reading finish refinement
The long-input reading area retains the word-meaning anchor, with16px lemma and14px meaning, a centered16px sentence at1.85 line height, and source/provider on two quiet centered lines. The next action is a compact text-and-arrow control; hover uses a low surface tone and keyboard focus is conveyed by a2px accent underline rather than a large rectangular outline. Long words and meanings wrap inside the reading measure. Expanded definitions no longer repeat the English-definition section; the primary English explanation remains.

Final alignment: center the vocabulary meaning row on the same axis as the sentence and provenance. The next action is an arrow-only44px target with no resting surface; a restrained circular outline remains for keyboard focus. Its accessible name and hover title identify next/retry and the keyboard shortcut.

Arrow-only follow-up: remove the next action's circle and resting/hover surfaces entirely. Keyboard focus brightens and thickens the arrow and moves it slightly; the44px target and accessible label remain.


## Practice continuity refinement
Next sentence uses a guarded150ms exit and320ms entrance on the practice content only; repeated presses do not skip sentences, and mode changes cancel the pending navigation. Listening completion restores natural letter widths through a360ms per-letter FLIP, retaining the centered word. Reduced motion changes state immediately. Group selection aligns with the left edge of the content rail while mode tabs stay centered; mobile keeps a compact row with its44px target.


## v1.0 release
The approved practice surface is the v1.0 baseline. Version and source download stay inside Settings > Data; no new practice-screen controls. Windows uses the same bundled web assets and its own persistent local profile, with a native window and no background service.

## v1.1 设置与键音修复
移除练习页左下角页脚签名及其点击事件。打开设置和切换分类时将活动面板滚动至顶部，保留面板内独立滚动。动效预览放在打字动效与退场选项上方，两项并排；预览退出粒子限定在预览容器内，切换分类时取消并清理，演示结束恢复静态单词。
新增绒感、木质、清亮、纸感四种本机合成键音；切换音色立即试听，提供单独试听按钮，关闭键音时禁用试听；使用同一 AudioContext 和音量控制。版本仍为 v1.1，未更新 EXE。

## 2026-11-06 · v1.1.1 网页微调
Operate，延续现有视觉。手感页先显示组合预览与动效/键音，再显示外观与读音；预览只在本组内吸附。中文正文16px/1.8、英英15px/1.7、词性14px，修正旧规则覆盖。默认轻弹170ms、小幅度；页签240ms，Tab位移360ms，共用 cubic-bezier(.16,1,.3,1)；不改用户保存的动效选项。来源与完整释义保持既有阅读位置。验证范围见 QA-v1.1.1.md。

单词字号：Ctrl＋滚轮连续缩放；普通滚轮仍滚动。60%—160%，优先保证当前单词完整显示，字区高度不变。设置滑杆及恢复默认用于无滚轮设备。预览同步显示字号；页面与释义字体不随之改变。

## 2026-11-06 · v1.1.2 动效微调
轻弹与碰撞回弹采用双字母接触传力、弹簧归位，幅度受限，280ms内恢复布局锚点；默认轻弹力度较小，碰撞回弹可选。退格单独140ms，整词退场360ms。预览覆盖输入、退格、补回与整词退场。验证见 QA-v1.1.2.md。

v1.1.2 r2：按体验反馈将删除退场回调至220ms，缓动(.22,.6,.35,1)，整词退场仍360ms。


## 2026-11-07 · v1.1.3 输入柔光
Operate，保留当前排版、字体、阅读与输入行为。作者性动作限定在当前字母后方：字母周围的光晕随按键舒展并回落，主题 accent 低透明度叠染，字母保持在最上层。单键 1150ms 尾迹，删除 680ms、更淡；每个字区最多六个标记，停打后全部释放，无空闲循环。渐变 alpha mask 使主题色向外自然透明，只做 transform/opacity，不使用动画滤镜。设置手感页可关闭晕染；无动效、系统减弱动效、后台页面停用。三种模式与预览共用触发入口。退格字母仍 220ms，整词统一 360ms，修复 forEach 索引误入删除参数。
