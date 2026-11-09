/*==============================================================================
 * 文件名称：ui.cpp
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：界面模块              负责人：雷子豪
 * 功能描述：基于 EGE 20.08 实现全部绘制工作。
 *           支持多主题切换、圆角控件、渐变背景、立体棋子、AI胜率显示。
 *============================================================================*/
#include <graphics.h>

#include "ui.h"
#include "ai.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#define FONT_FACE "微软雅黑"
/* 棋盘最后一根线的坐标（分 X / Y 两轴，避免用 X 原点算 Y 终点导致
 * 竖线与外框底线比实际棋盘短一格——这是“边缘线错位”的根源） */
#define BOARD_LAST_X  (UI_ORIGIN_X + (BOARD_SIZE - 1) * UI_CELL)
#define BOARD_LAST_Y  (UI_ORIGIN_Y + (BOARD_SIZE - 1) * UI_CELL)

static int g_theme = THEME_CLASSIC;

/* 阵型播报环形缓冲区(右侧面板滚动播报,保留最近 3 条) */
#define FORMATION_LOG_MAX 3
static char g_formationLog[FORMATION_LOG_MAX][128];
static int  g_formationLast = 0;   /* 最新一条的下标 */
static int  g_formationCount = 0;  /* 已推入条数(0..MAX) */

/* 当前阵型事件文案：仅在阵型新成型的那一步显示，下一步后刷新 */
static char g_formationEvent[256] = "";

/* 当前主题颜色变量（运行时由 ui_set_theme 更新） */
static color_t COLOR_BOARD_BG;
static color_t COLOR_GRID;
static color_t COLOR_LABEL;
static color_t COLOR_PANEL_BG;
static color_t COLOR_TEXT;
static color_t COLOR_TEXT_DIM;
static color_t COLOR_LINE;
static color_t COLOR_BTN;
static color_t COLOR_BTN_HOVER;
static color_t COLOR_BTN_ON;
static color_t COLOR_BTN_EDGE;
static color_t COLOR_ACCENT;
static color_t COLOR_MENU_BG1;
static color_t COLOR_MENU_BG2;

/* 固定颜色 */
#define COLOR_STONE_B    EGERGB(0x1A, 0x1A, 0x1A)
#define COLOR_STONE_W    EGERGB(0xF8, 0xF8, 0xF6)
#define COLOR_STONE_EDGE EGERGB(0x33, 0x33, 0x33)
#define COLOR_MARK       EGERGB(0xE0, 0x3A, 0x3A)
#define COLOR_WIN        EGERGB(0xFF, 0x44, 0x44)
#define COLOR_WARN       EGERGB(0xFF, 0xC7, 0x4D)
#define COLOR_INPUT_BG   EGERGB(0x1A, 0x1F, 0x28)

/* 主题按钮文字缓冲区（动态更新） */
static char themeButtonText[32] = "主题：经典木色";

static const UiButton MENU_BUTTONS[] =
{
    { 390, 200, 400, 48, BTN_PVP,          "双人对战"       },
    { 390, 256, 400, 48, BTN_PVE,          "人机对战"       },
    { 390, 312, 400, 48, BTN_NET_HOST,     "联网对战·创建房间" },
    { 390, 368, 400, 48, BTN_NET_JOIN,     "联网对战·加入房间" },
    { 357, 456,  66, 38, BTN_LEVEL_BEGINNER,    "入门" },
    { 437, 456,  66, 38, BTN_LEVEL_EASY,        "简单" },
    { 517, 456,  66, 38, BTN_LEVEL_NORMAL,      "普通" },
    { 597, 456,  66, 38, BTN_LEVEL_HARD,        "困难" },
    { 677, 456,  66, 38, BTN_LEVEL_MASTER,      "大师" },
    { 757, 456,  66, 38, BTN_LEVEL_GRANDMASTER, "棋圣" },
    { 408, 534, 170, 38, BTN_COLOR_BLACK,  "玩家执黑"     },
    { 602, 534, 170, 38, BTN_COLOR_WHITE,  "玩家执白"     },
    { 390, 600, 400, 44, BTN_MENU_LOAD,    "读取棋谱"       },
    { 390, 660, 400, 44, BTN_MENU_EXIT,    "退出游戏"       },
    {  60, 726, 220, 40, BTN_ALGO_BOOK,    "算法百科 · 产品介绍" },
    { 1050,  20, 112, 32, BTN_THEME,        themeButtonText  }
};
#define MENU_BUTTON_COUNT ((int)(sizeof(MENU_BUTTONS) / sizeof(MENU_BUTTONS[0])))

static const UiButton GAME_BUTTONS[] =
{
    { 790, 470, 360, 38, BTN_UNDO,    "悔  棋"     },
    { 790, 520, 360, 38, BTN_RESIGN,  "认  输"     },
    { 790, 570, 360, 38, BTN_SAVE,    "保存棋谱"   },
    { 790, 620, 360, 38, BTN_LOAD,    "读取棋谱"   },
    { 790, 670, 360, 38, BTN_RESTART, "重新开始"   },
    { 790, 720, 360, 38, BTN_BACK,    "返回主菜单" }
};
#define GAME_BUTTON_COUNT ((int)(sizeof(GAME_BUTTONS) / sizeof(GAME_BUTTONS[0])))

/*==============================================================================
 * 基础辅助函数
 *============================================================================*/
static void set_text_font(int size, color_t color)
{
    setfont(size, 0, FONT_FACE);
    setcolor(color);
    setbkmode(TRANSPARENT);
}

static void draw_text_center(int cx, int cy, const char *text)
{
    int w;
    int h;

    if (text == NULL) return;
    w = textwidth(text);
    h = textheight(text);
    outtextxy(cx - w / 2, cy - h / 2, text);
}

static void draw_text_wrapped(int x, int y, int maxWidth, int lineHeight,
                              int maxLine, const char *text)
{
    char line[128];
    int  used = 0;
    int  lineNo = 0;
    int  i = 0;

    if (text == NULL || text[0] == '\0') return;

    while (text[i] != '\0' && lineNo < maxLine)
    {
        int step = 1;
        if ((unsigned char)text[i] >= 0x81 && text[i + 1] != '\0') step = 2;
        if (used + step >= (int)sizeof(line)) break;

        memcpy(line + used, text + i, (size_t)step);
        used += step;
        line[used] = '\0';

        if (textwidth(line) > maxWidth)
        {
            used -= step;
            line[used] = '\0';
            outtextxy(x, y + lineNo * lineHeight, line);
            lineNo++;
            used = 0;
            line[0] = '\0';
            continue;
        }
        i += step;
    }

    if (used > 0 && lineNo < maxLine)
    {
        outtextxy(x, y + lineNo * lineHeight, line);
    }
}

/* 填充圆角矩形（用 bar + 四个角的 fillellipse 拼接） */
static void fill_round_rect(int x, int y, int w, int h, int r)
{
    color_t fc;
    if (r > h / 2) r = h / 2;
    if (r > w / 2) r = w / 2;
    fc = getfillcolor();
    setcolor(fc);  /* 边框色与填充色一致，消除四个角的可见圆圈 */
    bar(x + r, y, x + w - r, y + h);
    bar(x, y + r, x + w, y + h - r);
    fillellipse(x + r, y + r, r, r);
    fillellipse(x + w - r, y + r, r, r);
    fillellipse(x + r, y + h - r, r, r);
    fillellipse(x + w - r, y + h - r, r, r);
}

/* 绘制圆角矩形边框 */
static void stroke_round_rect(int x, int y, int w, int h, int r)
{
    if (r > h / 2) r = h / 2;
    if (r > w / 2) r = w / 2;
    line(x + r, y, x + w - r, y);
    line(x + r, y + h, x + w - r, y + h);
    line(x, y + r, x, y + h - r);
    line(x + w, y + r, x + w, y + h - r);
    arc(x + r, y + r, 90, 180, r);
    arc(x + w - r, y + r, 0, 90, r);
    arc(x + r, y + h - r, 180, 270, r);
    arc(x + w - r, y + h - r, 270, 360, r);
}

/* 绘制垂直渐变背景 */
static void draw_gradient_bg(color_t c1, color_t c2)
{
    ege_setpattern_lineargradient(0, 0, c1, 0, (float)UI_HEIGHT, c2);
    bar(0, 0, UI_WIDTH, UI_HEIGHT);
    setfillcolor(COLOR_PANEL_BG);  /* 恢复纯色填充 */
}

/*==============================================================================
 * 胜率条
 *============================================================================*/
static void draw_winrate_bar(int x, int y, int w, int h, const Board *board)
{
    int blackRate;
    int whiteRate;
    int blackW;
    char text[32];

    if (board->status == GAME_BLACK_WIN)      { blackRate = 100; }
    else if (board->status == GAME_WHITE_WIN) { blackRate = 0;   }
    else if (board->status == GAME_DRAW)       { blackRate = 50;  }
    else
    {
        int eval = ai_evaluate(board, CELL_BLACK);
        double ratio = 1.0 / (1.0 + exp(-eval / 4000.0));
        blackRate = (int)(ratio * 100);
        if (blackRate < 1) blackRate = 1;
        if (blackRate > 99) blackRate = 99;
    }
    whiteRate = 100 - blackRate;

    /* 背景 */
    setfillcolor(COLOR_BTN);
    bar(x, y, x + w, y + h);

    /* 黑方占比 */
    blackW = w * blackRate / 100;
    if (blackW > 0)
    {
        setfillcolor(COLOR_STONE_B);
        bar(x, y, x + blackW, y + h);
    }

    /* 中间 50% 标记线 */
    setcolor(COLOR_LINE);
    setlinewidth(1);
    line(x + w / 2, y, x + w / 2, y + h);

    /* 文字 */
    set_text_font(12, COLOR_TEXT);
    snprintf(text, sizeof(text), "黑 %d%%", blackRate);
    outtextxy(x + 6, y + (h - textheight(text)) / 2, text);

    snprintf(text, sizeof(text), "白 %d%%", whiteRate);
    {
        int tw = textwidth(text);
        set_text_font(12, COLOR_TEXT_DIM);
        outtextxy(x + w - tw - 6, y + (h - textheight(text)) / 2, text);
    }
}

/*==============================================================================
 * 按钮绘制（圆角）
 *============================================================================*/
static void draw_button(const UiButton *button, int hover, int selected)
{
    color_t back;
    color_t edge;
    color_t textColor = COLOR_TEXT;

    if (selected)
    {
        back      = COLOR_BTN_ON;
        edge      = COLOR_ACCENT;
        textColor = COLOR_TEXT;
    }
    else if (hover)
    {
        /* 悬停:底色保持不变,仅边框加粗 + 文字高亮,避免整块背景变亮 */
        back      = COLOR_BTN;
        edge      = COLOR_ACCENT;
        textColor = COLOR_ACCENT;
    }
    else
    {
        back = COLOR_BTN;
        edge = COLOR_BTN_EDGE;
    }

    setfillcolor(back);
    fill_round_rect(button->x, button->y, button->w, button->h, 4);

    setcolor(edge);
    setlinewidth(hover ? 2 : 1);
    stroke_round_rect(button->x, button->y, button->w, button->h, 4);
    setlinewidth(1);

    set_text_font(20, textColor);
    draw_text_center(button->x + button->w / 2, button->y + button->h / 2, button->text);
}

/* 紧凑按钮(难度弹窗内风格按钮专用):字号 16,适配「系别·风格」的长文字 */
static void draw_style_button(const UiButton *button, int hover, int selected)
{
    color_t back;
    color_t edge;
    color_t textColor = COLOR_TEXT;

    if (selected)
    {
        back      = COLOR_BTN_ON;
        edge      = COLOR_ACCENT;
        textColor = COLOR_TEXT;
    }
    else if (hover)
    {
        back      = COLOR_BTN;
        edge      = COLOR_ACCENT;
        textColor = COLOR_ACCENT;
    }
    else
    {
        back = COLOR_BTN;
        edge = COLOR_BTN_EDGE;
    }

    setfillcolor(back);
    fill_round_rect(button->x, button->y, button->w, button->h, 4);
    setcolor(edge);
    setlinewidth(hover ? 2 : 1);
    stroke_round_rect(button->x, button->y, button->w, button->h, 4);
    setlinewidth(1);

    set_text_font(16, textColor);
    draw_text_center(button->x + button->w / 2, button->y + button->h / 2,
                     button->text);
}

/*==============================================================================
 * 棋子绘制（立体高光）
 *============================================================================*/
static void draw_stone(int row, int col, int player)
{
    int cx = UI_ORIGIN_X + col * UI_CELL;
    int cy = UI_ORIGIN_Y + row * UI_CELL;
    int r  = UI_STONE_R;

    /* 底部投影：黑棋深、白棋浅棕，让棋子从木纹背景中浮起 */
    if (player == CELL_BLACK)
    {
        setfillcolor(EGERGB(0x30, 0x28, 0x20));
        fillellipse(cx, cy + 2, r, r);
    }
    else
    {
        setfillcolor(EGERGB(0xC6, 0xA8, 0x7C));
        fillellipse(cx, cy + 2, r, r);
    }

    /* 棋身主体 + 描边：黑棋深色细描边，白棋加粗木纹色描边（木底上轮廓分明） */
    setfillcolor((player == CELL_BLACK) ? COLOR_STONE_B : COLOR_STONE_W);
    if (player == CELL_BLACK)
    {
        setcolor(EGERGB(0x10, 0x10, 0x10));
        setlinewidth(1);
    }
    else
    {
        setcolor(EGERGB(0x87, 0x6B, 0x43));
        setlinewidth(2);
    }
    fillellipse(cx, cy, r, r);

    /* 黑棋左上高光，增强立体感 */
    if (player == CELL_BLACK)
    {
        setfillcolor(EGERGB(0x5C, 0x5C, 0x5C));
        fillellipse(cx - r / 3, cy - r / 3, r * 2 / 5, r * 2 / 5);
    }
    setlinewidth(1);
}

/*==============================================================================
 * 棋盘绘制
 *============================================================================*/
static void draw_board_grid(void)
{
    static const int STAR[5][2] = { {3,3},{3,11},{11,3},{11,11},{7,7} };
    char label[8];
    int  i;

    /* 棋盘区：垂直渐变背景（上浅下深，木色系），模拟真实木棋盘的质感 */
    ege_setpattern_lineargradient(0, 0, EGERGB(0xF2, 0xDD, 0xB6),
                                  0, (float)UI_HEIGHT, COLOR_BOARD_BG);
    bar(0, 0, UI_PANEL_X - 1, UI_HEIGHT - 1);
    setfillcolor(COLOR_BOARD_BG);

    setcolor(COLOR_GRID);
    setlinewidth(1);
    for (i = 0; i < BOARD_SIZE; i++)
    {
        int posX = UI_ORIGIN_X + i * UI_CELL;
        int posY = UI_ORIGIN_Y + i * UI_CELL;
        line(UI_ORIGIN_X, posY, BOARD_LAST_X, posY);   /* 横线：贯穿棋盘整宽 */
        line(posX, UI_ORIGIN_Y, posX, BOARD_LAST_Y);   /* 竖线：贯穿棋盘整高 */
    }

    setlinewidth(3);
    setcolor(EGERGB(0x2E, 0x1F, 0x0C));
    rectangle(UI_ORIGIN_X, UI_ORIGIN_Y, BOARD_LAST_X, BOARD_LAST_Y);  /* 外框四边对齐首尾线，加粗显木框感 */
    setlinewidth(1);

    setfillcolor(COLOR_GRID);
    for (i = 0; i < 5; i++)
    {
        fillellipse(UI_ORIGIN_X + STAR[i][1] * UI_CELL, UI_ORIGIN_Y + STAR[i][0] * UI_CELL, 5, 5);
    }

    set_text_font(14, COLOR_LABEL);
    for (i = 0; i < BOARD_SIZE; i++)
    {
        sprintf(label, "%c", (char)('A' + i));
        draw_text_center(UI_ORIGIN_X + i * UI_CELL, UI_ORIGIN_Y - 22, label);
        sprintf(label, "%d", i + 1);
        draw_text_center(UI_ORIGIN_X - 24, UI_ORIGIN_Y + i * UI_CELL, label);
    }
}

static void draw_stones(const Board *board)
{
    const Move *last;
    int row;
    int col;
    int i;

    for (row = 0; row < BOARD_SIZE; row++)
    {
        for (col = 0; col < BOARD_SIZE; col++)
        {
            int cell = board_get(board, row, col);
            if (cell != CELL_EMPTY) draw_stone(row, col, cell);
        }
    }

    last = board_last_move(board);
    if (last != NULL)
    {
        setfillcolor(COLOR_MARK);
        setcolor(COLOR_MARK);
        fillellipse(UI_ORIGIN_X + last->col * UI_CELL, UI_ORIGIN_Y + last->row * UI_CELL, 4, 4);
    }

    if (board->hasWinLine)
    {
        setcolor(COLOR_WIN);
        setlinewidth(3);
        line(UI_ORIGIN_X + board->winLine[0][1] * UI_CELL, UI_ORIGIN_Y + board->winLine[0][0] * UI_CELL,
             UI_ORIGIN_X + board->winLine[WIN_COUNT-1][1] * UI_CELL, UI_ORIGIN_Y + board->winLine[WIN_COUNT-1][0] * UI_CELL);
        setlinewidth(2);
        for (i = 0; i < WIN_COUNT; i++)
        {
            circle(UI_ORIGIN_X + board->winLine[i][1] * UI_CELL, UI_ORIGIN_Y + board->winLine[i][0] * UI_CELL, UI_STONE_R + 2);
        }
        setlinewidth(1);
    }
}

static void draw_hover_stone(const Board *board, int row, int col)
{
    if (!board_in_range(row, col)) return;
    if (board_can_place(board, row, col) != PLACE_OK) return;

    setcolor((board->current == CELL_BLACK) ? COLOR_STONE_B : COLOR_STONE_W);
    setlinewidth(2);
    circle(UI_ORIGIN_X + col * UI_CELL, UI_ORIGIN_Y + row * UI_CELL, UI_STONE_R);
    setlinewidth(1);
}

static const char *status_line(const Board *board)
{
    switch (board->status)
    {
    case GAME_BLACK_WIN: return "对局结束：黑方获胜";
    case GAME_WHITE_WIN: return "对局结束：白方获胜";
    case GAME_DRAW:      return "对局结束：棋盘已满，和棋";
    default:             return "对局进行中";
    }
}

/*==============================================================================
 * 右侧信息面板
 *============================================================================*/
static void draw_panel(const Board *board, const GameInfo *info,
                       const char *message, int thinking)
{
    const int x0 = UI_PANEL_X + 28;
    const int panelW = UI_WIDTH - UI_PANEL_X - 56;
    char text[128];
    int  y;

    setfillcolor(COLOR_PANEL_BG);
    bar(UI_PANEL_X, 0, UI_WIDTH - 1, UI_HEIGHT - 1);

    set_text_font(30, COLOR_TEXT);
    outtextxy(x0, 26, "五子棋对战");
    set_text_font(16, COLOR_TEXT_DIM);
    outtextxy(x0, 68, "第15小组  程序设计实践");

    setcolor(COLOR_LINE);
    line(x0, 96, UI_WIDTH - 28, 96);

    y = 112;
    set_text_font(18, COLOR_TEXT);

    if (info->mode == MODE_NET)
        sprintf(text, "对局模式：联网对战");
    else
        sprintf(text, "对局模式：%s", (info->mode == MODE_PVE) ? "人机对战" : "双人对战");
    outtextxy(x0, y, text);
    y += 26;

    if (info->mode == MODE_PVE)
    {
        sprintf(text, "难度等级：%s", ai_level_text(info->level));
        outtextxy(x0, y, text);
        y += 26;
        sprintf(text, "AI 风格：%s", ai_style_text((info->level >= LEVEL_HARD) ? info->style : STYLE_NORMAL));
        outtextxy(x0, y, text);
        y += 26;
        sprintf(text, "玩家执子：%s", (info->humanColor == CELL_BLACK) ? "黑棋" : "白棋");
        outtextxy(x0, y, text);
        y += 26;
    }
    if (info->mode == MODE_NET)
    {
        sprintf(text, "本机执子：%s", (info->humanColor == CELL_BLACK) ? "黑棋" : "白棋");
        outtextxy(x0, y, text);
        y += 26;
    }

    sprintf(text, "已走手数：%d", board->moveCount);
    outtextxy(x0, y, text);
    y += 26;

    if (board->status == GAME_PLAYING)
    {
        setfillcolor((board->current == CELL_BLACK) ? COLOR_STONE_B : COLOR_STONE_W);
        setcolor(COLOR_STONE_EDGE);
        fillellipse(x0 + 10, y + 9, 9, 9);
        set_text_font(18, COLOR_TEXT);
        sprintf(text, "  当前回合：%s", (board->current == CELL_BLACK) ? "黑方" : "白方");
        outtextxy(x0 + 14, y - 2, text);
    }
    else
    {
        set_text_font(18, COLOR_WARN);
        outtextxy(x0, y - 2, status_line(board));
    }
    y += 28;

    /* AI 胜率条 */
    set_text_font(15, COLOR_TEXT);
    outtextxy(x0, y, "AI 胜率评估");
    y += 18;
    draw_winrate_bar(x0, y, panelW, 20, board);
    y += 28;

    setcolor(COLOR_LINE);
    line(x0, y, UI_WIDTH - 28, y);

    /* 阵型介绍：仅在阵型新成型的那一步显示事件文案，下一步后自动刷新 */
    {
        int fy = y + 10;

        set_text_font(15, COLOR_ACCENT);
        outtextxy(x0, fy, "阵型介绍");
        fy += 20;

        if (g_formationEvent[0] != '\0')
        {
            set_text_font(14, COLOR_WARN);
            draw_text_wrapped(x0, fy, panelW, 36, 2, g_formationEvent);
            fy += 36;
        }
        else
        {
            set_text_font(14, COLOR_TEXT_DIM);
            outtextxy(x0, fy, "暂无新阵法成型");
            fy += 18;
        }
        y = fy + 2;
        setcolor(COLOR_LINE);
        line(x0, y, UI_WIDTH - 28, y);
    }

    /* 消息区 */
    {
        int my = y + 10;

        if (thinking)
        {
            set_text_font(14, COLOR_WARN);
            draw_text_wrapped(x0, my, panelW, 18, 1, "电脑正在思考，请稍候……");
        }
        else if (message != NULL && message[0] != '\0')
        {
            set_text_font(14, COLOR_TEXT_DIM);
            draw_text_wrapped(x0, my, panelW, 18, 1, message);
        }
    }
}

void ui_set_formation_report(const char *text)
{
    int i;
    g_formationLast = (g_formationLast + 1) % FORMATION_LOG_MAX;
    g_formationLog[g_formationLast][0] = '\0';
    if (text != NULL)
    {
        strncpy(g_formationLog[g_formationLast], text, sizeof(g_formationLog[0]) - 1);
        g_formationLog[g_formationLast][sizeof(g_formationLog[0]) - 1] = '\0';
        if (g_formationCount < FORMATION_LOG_MAX) g_formationCount++;
    }
    else
    {
        /* text 为 NULL 表示清空播报 */
        for (i = 0; i < FORMATION_LOG_MAX; i++) g_formationLog[i][0] = '\0';
        g_formationCount = 0;
        g_formationLast  = 0;
    }
}

/* 设置当前阵型事件文案（仅在成型那一步显示，传NULL清空） */
void ui_set_formation_event(const char *text)
{
    g_formationEvent[0] = '\0';
    if (text != NULL && text[0] != '\0')
    {
        strncpy(g_formationEvent, text, sizeof(g_formationEvent) - 1);
        g_formationEvent[sizeof(g_formationEvent) - 1] = '\0';
    }
}

/*==============================================================================
 * 初始化 / 关闭
 *============================================================================*/
void ui_init(void)
{
    initgraph(UI_WIDTH, UI_HEIGHT);
    setrendermode(RENDER_MANUAL);   /* 双缓冲:绘制先进缓存,统一刷新(ui_flush)上屏,消除屏闪 */
    setcaption("五子棋对战游戏  —  第15小组");
    ui_set_theme(THEME_CLASSIC);
    cleardevice();
}

void ui_close(void)
{
    closegraph();
}

int ui_menu_buttons(const UiButton **buttons)
{
    if (buttons != NULL) *buttons = MENU_BUTTONS;
    return MENU_BUTTON_COUNT;
}

int ui_game_buttons(const UiButton **buttons)
{
    if (buttons != NULL) *buttons = GAME_BUTTONS;
    return GAME_BUTTON_COUNT;
}

/*==============================================================================
 * 主菜单绘制（渐变背景 + 装饰）
 *============================================================================*/
void ui_draw_menu(const GameInfo *info, int hoverId)
{
    int i;

    draw_gradient_bg(COLOR_MENU_BG1, COLOR_MENU_BG2);

    /* 装饰：左上角半透明棋子 */
    {
        setfillcolor(COLOR_STONE_B);
        fillellipse(60, 80, 22, 22);
        setfillcolor(COLOR_STONE_W);
        fillellipse(110, 110, 18, 18);
        setfillcolor(COLOR_STONE_B);
        fillellipse(880, 560, 20, 20);
        setfillcolor(COLOR_STONE_W);
        fillellipse(840, 590, 16, 16);
    }

    /* 标题（首界面大标题，字体：楷体） */
    set_text_font(56, COLOR_TEXT);
    setfont(56, 0, "楷体");
    draw_text_center(UI_WIDTH / 2, 70, "五子棋");

    /* 标题装饰线 */
    setcolor(COLOR_ACCENT);
    setlinewidth(2);
    line(UI_WIDTH / 2 - 150, 122, UI_WIDTH / 2 - 40, 122);
    line(UI_WIDTH / 2 + 40, 122, UI_WIDTH / 2 + 150, 122);
    setfillcolor(COLOR_ACCENT);
    fillellipse(UI_WIDTH / 2, 122, 5, 5);
    setlinewidth(1);

    set_text_font(18, COLOR_TEXT_DIM);
    draw_text_center(UI_WIDTH / 2, 156, "第15小组   程序设计实践   双人·人机·联网对战");

    /* 难度 / 执子 标签 */
    set_text_font(19, COLOR_TEXT_DIM);
    draw_text_center(UI_WIDTH / 2, 434, "AI 难度（人机对战时生效，点击查看算法介绍）");
    set_text_font(19, COLOR_TEXT_DIM);
    draw_text_center(UI_WIDTH / 2, 512, "执子颜色（黑方先行）");

    for (i = 0; i < MENU_BUTTON_COUNT; i++)
    {
        const UiButton *button = &MENU_BUTTONS[i];
        int selected = 0;

        switch (button->id)
        {
        case BTN_LEVEL_BEGINNER: selected = (info->level == LEVEL_BEGINNER); break;
        case BTN_LEVEL_EASY:     selected = (info->level == LEVEL_EASY);     break;
        case BTN_LEVEL_NORMAL:   selected = (info->level == LEVEL_NORMAL);   break;
        case BTN_LEVEL_HARD:     selected = (info->level == LEVEL_HARD);     break;
        case BTN_LEVEL_MASTER:   selected = (info->level == LEVEL_MASTER);   break;
        case BTN_LEVEL_GRANDMASTER: selected = (info->level == LEVEL_GRANDMASTER); break;
        case BTN_COLOR_BLACK:    selected = (info->humanColor == CELL_BLACK); break;
        case BTN_COLOR_WHITE:    selected = (info->humanColor == CELL_WHITE); break;
        default: selected = 0; break;
        }

        draw_button(button, (button->id == hoverId), selected);
    }
}

/*==============================================================================
 * 对局界面绘制
 *============================================================================*/
void ui_draw_game(const Board *board, const GameInfo *info,
                  const char *message, int hoverId,
                  int hoverRow, int hoverCol, int thinking,
                  const int *hlRows, const int *hlCols, int hlCount)
{
    int i;

    if (board == NULL || info == NULL) return;

    draw_board_grid();
    draw_stones(board);

    /* 阵型高亮：阵型新成型时高亮组成棋子（金色外圈光环） */
    if (hlCount > 0 && hlRows != NULL && hlCols != NULL)
    {
        setcolor(EGERGB(255, 200, 0));
        setlinewidth(3);
        for (i = 0; i < hlCount; i++)
        {
            circle(UI_ORIGIN_X + hlCols[i] * UI_CELL,
                   UI_ORIGIN_Y + hlRows[i] * UI_CELL,
                   UI_STONE_R + 4);
        }
        setlinewidth(1);
    }

    if (!thinking && board->status == GAME_PLAYING)
    {
        draw_hover_stone(board, hoverRow, hoverCol);
    }

    draw_panel(board, info, message, thinking);

    for (i = 0; i < GAME_BUTTON_COUNT; i++)
    {
        draw_button(&GAME_BUTTONS[i], (GAME_BUTTONS[i].id == hoverId), 0);
    }
}

void ui_flush(void)
{
    delay_fps(60);
}

/*==============================================================================
 * 棋谱浏览界面
 *============================================================================*/
void ui_draw_browser(const UiRow *rows, int rowCount, int hoverId,
                     const char *message)
{
    static const UiButton backButton = { 470, 730, 240, 40, BTN_LOAD_BACK, "返  回" };
    const int rowTop = 128;
    const int rowGap = 6;
    int i;

    draw_gradient_bg(COLOR_MENU_BG1, COLOR_MENU_BG2);

    set_text_font(34, COLOR_TEXT);
    draw_text_center(UI_WIDTH / 2, 56, "读取棋谱");

    set_text_font(16, COLOR_TEXT_DIM);
    draw_text_center(UI_WIDTH / 2, 94, "点击一条存档即可载入，按返回键回到上一界面");

    setcolor(COLOR_LINE);
    line(80, 116, UI_WIDTH - 80, 116);

    for (i = 0; i < rowCount; i++)
    {
        int y = rowTop + i * (52 + rowGap);
        int hover = (rows[i].id == hoverId);
        if (rows[i].text == NULL) break;

        setfillcolor(hover ? COLOR_BTN_HOVER : COLOR_BTN);
        fill_round_rect(80, y, UI_WIDTH - 160, 52, 10);

        set_text_font(20, COLOR_TEXT);
        outtextxy(104, y + 6, rows[i].text);
        if (rows[i].subtext != NULL && rows[i].subtext[0] != '\0')
        {
            set_text_font(16, COLOR_TEXT_DIM);
            outtextxy(104, y + 30, rows[i].subtext);
        }
    }

    draw_button(&backButton, (hoverId == BTN_LOAD_BACK), 0);

    if (message != NULL && message[0] != '\0')
    {
        set_text_font(16, COLOR_WARN);
        draw_text_center(UI_WIDTH / 2, 782, message);
    }
}

/*==============================================================================
 * 联网等待界面
 *============================================================================*/
void ui_draw_net_wait(int isHost, const char *status, const char *localIp,
                      const char *targetIp, int hoverId)
{
    static const UiButton cancelButton = { 490, 470, 200, 44, BTN_NET_CANCEL, "返  回" };
    static int animFrame = 0;
    char text[160];

    animFrame++;

    draw_gradient_bg(COLOR_MENU_BG1, COLOR_MENU_BG2);

    /* 标题 */
    set_text_font(34, COLOR_TEXT);
    draw_text_center(UI_WIDTH / 2, 78, isHost ? "联网对战 · 创建房间" : "联网对战 · 加入房间");

    setcolor(COLOR_LINE);
    setlinewidth(1);
    line(220, 128, UI_WIDTH - 220, 128);

    if (isHost)
    {
        set_text_font(18, COLOR_TEXT_DIM);
        draw_text_center(UI_WIDTH / 2, 160, "房间已创建，等待对手加入");

        {
            int cx = UI_WIDTH / 2;
            int cy = 220;
            int angle = (animFrame * 5) % 360;
            setcolor(COLOR_BTN_EDGE);
            setlinewidth(2);
            circle(cx, cy, 26);
            setcolor(COLOR_ACCENT);
            setlinewidth(3);
            arc(cx, cy, angle, angle + 90, 26);
            setlinewidth(1);
        }

        set_text_font(20, COLOR_TEXT);
        draw_text_center(UI_WIDTH / 2, 280, status != NULL ? status : "");

        set_text_font(22, COLOR_WARN);
        snprintf(text, sizeof(text), "本机 IP：%s", localIp);
        draw_text_center(UI_WIDTH / 2, 330, text);
        set_text_font(18, COLOR_TEXT);
        draw_text_center(UI_WIDTH / 2, 362, "端口：8888");

        set_text_font(15, COLOR_TEXT_DIM);
        draw_text_center(UI_WIDTH / 2, 410, "请把上面的 IP 告诉对方，让对方选择");
        draw_text_center(UI_WIDTH / 2, 432, "「联网对战·加入房间」并输入该 IP");
    }
    else
    {
        set_text_font(18, COLOR_TEXT_DIM);
        draw_text_center(UI_WIDTH / 2, 160, "正在连接服务器，请稍候");

        {
            int cx = UI_WIDTH / 2;
            int cy = 220;
            int angle = (animFrame * 5) % 360;
            setcolor(COLOR_BTN_EDGE);
            setlinewidth(2);
            circle(cx, cy, 26);
            setcolor(COLOR_ACCENT);
            setlinewidth(3);
            arc(cx, cy, angle, angle + 90, 26);
            setlinewidth(1);
        }

        set_text_font(20, COLOR_TEXT);
        draw_text_center(UI_WIDTH / 2, 280, status != NULL ? status : "正在连接……");

        if (targetIp != NULL && targetIp[0] != '\0')
        {
            set_text_font(16, COLOR_TEXT_DIM);
            snprintf(text, sizeof(text), "目标服务器：%s : 8888", targetIp);
            draw_text_center(UI_WIDTH / 2, 320, text);
        }

        set_text_font(15, COLOR_TEXT_DIM);
        draw_text_center(UI_WIDTH / 2, 380, "连接成功后将自动开始对局");
        draw_text_center(UI_WIDTH / 2, 404, "若长时间无响应，请检查 IP 地址与网络");
    }

    draw_button(&cancelButton, (hoverId == BTN_NET_CANCEL), 0);
}

/*==============================================================================
 * 加入房间输入界面
 *============================================================================*/
void ui_draw_net_join(const char *ip, int cursorVisible, int hoverId,
                      const char *message)
{
    static const UiButton connectButton = { 390, 355, 180, 44, BTN_NET_CONNECT, "连  接" };
    static const UiButton backButton    = { 610, 355, 180, 44, BTN_JOIN_BACK,    "返  回" };
    const int inputX = 350;
    const int inputY = 235;
    const int inputW = 480;
    const int inputH = 52;
    int ipLen;

    draw_gradient_bg(COLOR_MENU_BG1, COLOR_MENU_BG2);

    set_text_font(34, COLOR_TEXT);
    draw_text_center(UI_WIDTH / 2, 78, "联网对战 · 加入房间");

    setcolor(COLOR_LINE);
    setlinewidth(1);
    line(220, 128, UI_WIDTH - 220, 128);

    set_text_font(16, COLOR_TEXT_DIM);
    draw_text_center(UI_WIDTH / 2, 165, "请输入房主的 IP 地址，连接成功后自动开始对局");

    set_text_font(15, COLOR_TEXT_DIM);
    outtextxy(inputX, inputY - 26, "服务器 IP");

    /* 输入框 */
    setfillcolor(COLOR_INPUT_BG);
    fill_round_rect(inputX, inputY, inputW, inputH, 8);
    setcolor(COLOR_ACCENT);
    setlinewidth(2);
    rectangle(inputX, inputY, inputX + inputW, inputY + inputH);
    setlinewidth(1);

    ipLen = (ip != NULL) ? (int)strlen(ip) : 0;
    set_text_font(22, COLOR_TEXT);
    if (ipLen > 0)
    {
        outtextxy(inputX + 16, inputY + (inputH - textheight("0")) / 2, ip);
    }
    else
    {
        set_text_font(20, COLOR_TEXT_DIM);
        outtextxy(inputX + 16, inputY + (inputH - textheight("0")) / 2, "例如 192.168.1.100");
    }

    if (cursorVisible)
    {
        int cursorX;
        set_text_font(22, COLOR_TEXT);
        if (ipLen > 0 && ip != NULL)
            cursorX = inputX + 16 + textwidth(ip);
        else
            cursorX = inputX + 16;
        setfillcolor(COLOR_TEXT);
        bar(cursorX, inputY + 12, cursorX + 2, inputY + inputH - 12);
    }

    set_text_font(14, COLOR_TEXT_DIM);
    draw_text_center(UI_WIDTH / 2, inputY + inputH + 22, "端口：8888  （端口固定，无需输入）");

    draw_button(&connectButton, (hoverId == BTN_NET_CONNECT), 0);
    draw_button(&backButton, (hoverId == BTN_JOIN_BACK), 0);

    if (message != NULL && message[0] != '\0')
    {
        set_text_font(15, COLOR_WARN);
        draw_text_center(UI_WIDTH / 2, 430, message);
    }

    set_text_font(13, COLOR_TEXT_DIM);
    draw_text_center(UI_WIDTH / 2, 590, "提示：IP 格式如 192.168.1.100；本机测试可输入 127.0.0.1；回车快速连接");
}

/*==============================================================================
 * 主题管理
 *============================================================================*/
void ui_set_theme(int theme)
{
    if (theme < 0 || theme >= THEME_COUNT) theme = 0;
    g_theme = theme;

    switch (theme)
    {
    case THEME_DARK:   /* 暗夜 · 冷蓝紫调 */
        COLOR_BOARD_BG  = EGERGB(0x33, 0x37, 0x42);
        COLOR_GRID      = EGERGB(0x66, 0x6E, 0x7E);
        COLOR_LABEL     = EGERGB(0x9A, 0xA2, 0xB2);
        COLOR_PANEL_BG  = EGERGB(0x14, 0x15, 0x1E);
        COLOR_TEXT      = EGERGB(0xF4, 0xF5, 0xFA);
        COLOR_TEXT_DIM  = EGERGB(0x8E, 0x93, 0xA8);
        COLOR_LINE      = EGERGB(0x3A, 0x3E, 0x4E);
        COLOR_BTN       = EGERGB(0x22, 0x24, 0x30);
        COLOR_BTN_HOVER = EGERGB(0x34, 0x38, 0x4C);
        COLOR_BTN_ON    = EGERGB(0x7C, 0x6C, 0xF0);
        COLOR_BTN_EDGE  = EGERGB(0x46, 0x4B, 0x5E);
        COLOR_ACCENT    = EGERGB(0x8B, 0x7B, 0xF6);
        COLOR_MENU_BG1  = EGERGB(0x0C, 0x0D, 0x16);
        COLOR_MENU_BG2  = EGERGB(0x22, 0x1E, 0x3A);
        break;
    case THEME_JADE:   /* 翡翠绿 · 翠绿+琥珀 */
        COLOR_BOARD_BG  = EGERGB(0xCD, 0xE8, 0xCE);
        COLOR_GRID      = EGERGB(0x2F, 0x7D, 0x35);
        COLOR_LABEL     = EGERGB(0x1E, 0x5E, 0x24);
        COLOR_PANEL_BG  = EGERGB(0x18, 0x38, 0x28);
        COLOR_TEXT      = EGERGB(0xF0, 0xF8, 0xEA);
        COLOR_TEXT_DIM  = EGERGB(0xA8, 0xD4, 0xAA);
        COLOR_LINE      = EGERGB(0x2E, 0x58, 0x3C);
        COLOR_BTN       = EGERGB(0x26, 0x4C, 0x34);
        COLOR_BTN_HOVER = EGERGB(0x3C, 0x6E, 0x4A);
        COLOR_BTN_ON    = EGERGB(0xE8, 0xA8, 0x2E);
        COLOR_BTN_EDGE  = EGERGB(0x4A, 0x82, 0x58);
        COLOR_ACCENT    = EGERGB(0xE8, 0xA8, 0x2E);
        COLOR_MENU_BG1  = EGERGB(0x0D, 0x24, 0x18);
        COLOR_MENU_BG2  = EGERGB(0x1B, 0x3E, 0x2A);
        break;
    case THEME_CLASSIC:  /* 经典木色 · 暖木+金棕 */
        COLOR_BOARD_BG  = EGERGB(0xD9, 0xB9, 0x8D);
        COLOR_GRID      = EGERGB(0x3D, 0x2B, 0x13);
        COLOR_LABEL     = EGERGB(0x6B, 0x4C, 0x28);
        COLOR_PANEL_BG  = EGERGB(0x26, 0x2B, 0x35);
        COLOR_TEXT      = EGERGB(0xF2, 0xF3, 0xF5);
        COLOR_TEXT_DIM  = EGERGB(0x9A, 0xA4, 0xB8);
        COLOR_LINE      = EGERGB(0x3C, 0x44, 0x52);
        COLOR_BTN       = EGERGB(0x37, 0x40, 0x50);
        COLOR_BTN_HOVER = EGERGB(0x4C, 0x5A, 0x72);
        COLOR_BTN_ON    = EGERGB(0xC8, 0x89, 0x3B);
        COLOR_BTN_EDGE  = EGERGB(0x59, 0x64, 0x78);
        COLOR_ACCENT    = EGERGB(0xC8, 0x89, 0x3B);
        COLOR_MENU_BG1  = EGERGB(0x1A, 0x1F, 0x28);
        COLOR_MENU_BG2  = EGERGB(0x3A, 0x2E, 0x20);
        break;
    case THEME_OCEAN:   /* 海洋蓝 · 深海蓝+天青 */
        COLOR_BOARD_BG  = EGERGB(0xBF, 0xDB, 0xE8);
        COLOR_GRID      = EGERGB(0x1E, 0x5A, 0x78);
        COLOR_LABEL     = EGERGB(0x15, 0x44, 0x5E);
        COLOR_PANEL_BG  = EGERGB(0x0E, 0x28, 0x38);
        COLOR_TEXT      = EGERGB(0xEA, 0xF6, 0xFB);
        COLOR_TEXT_DIM  = EGERGB(0x8F, 0xC2, 0xDA);
        COLOR_LINE      = EGERGB(0x1E, 0x3E, 0x52);
        COLOR_BTN       = EGERGB(0x16, 0x36, 0x48);
        COLOR_BTN_HOVER = EGERGB(0x26, 0x52, 0x6C);
        COLOR_BTN_ON    = EGERGB(0x38, 0xBD, 0xF8);
        COLOR_BTN_EDGE  = EGERGB(0x3A, 0x66, 0x80);
        COLOR_ACCENT    = EGERGB(0x38, 0xBD, 0xF8);
        COLOR_MENU_BG1  = EGERGB(0x08, 0x1A, 0x26);
        COLOR_MENU_BG2  = EGERGB(0x14, 0x3A, 0x52);
        break;
    }

    snprintf(themeButtonText, sizeof(themeButtonText), "主题：%s", ui_theme_name(theme));
    setbkcolor(COLOR_PANEL_BG);
}

int ui_get_theme(void)
{
    return g_theme;
}

const char *ui_theme_name(int theme)
{
    switch (theme)
    {
    case THEME_CLASSIC: return "经典木色";
    case THEME_DARK:    return "暗夜";
    case THEME_JADE:    return "翡翠绿";
    case THEME_OCEAN:   return "海洋蓝";
    case THEME_VIOLET:  return "紫罗兰";
    case THEME_SUNSET:  return "落日橙";
    default:             return "未知";
    }
}

/*==============================================================================
 * 棋谱回放界面
 *============================================================================*/
void ui_draw_replay(const Board *board, const GameInfo *info,
                    int replayIndex, int replayCount, int hoverId,
                    const int *hlRows, const int *hlCols, int hlCount)
{
    static const UiButton replayButtons[] =
    {
        { 790, 430, 170, 38, BTN_REPLAY_FIRST, "首步" },
        { 980, 430, 170, 38, BTN_REPLAY_PREV,  "上一步" },
        { 790, 476, 170, 38, BTN_REPLAY_NEXT,  "下一步" },
        { 980, 476, 170, 38, BTN_REPLAY_LAST,  "末步" },
        { 790, 540, 360, 42, BTN_REPLAY_BACK,  "返回棋谱列表" }
    };
    const int btnCount = 5;
    const int x0 = UI_PANEL_X + 28;
    const int barX = 790;
    const int barY = 356;
    const int barW = 360;
    const int barH = 12;
    char text[128];
    int  i;
    int  fillW;

    if (board == NULL || info == NULL) return;

    /* 棋盘 */
    draw_board_grid();
    draw_stones(board);

    /* 阵型高亮：仅在阵型新成型的那一步高亮棋子（金色外圈光环） */
    if (hlCount > 0 && hlRows != NULL && hlCols != NULL)
    {
        setcolor(EGERGB(255, 200, 0));
        setlinewidth(3);
        for (i = 0; i < hlCount; i++)
        {
            circle(UI_ORIGIN_X + hlCols[i] * UI_CELL,
                   UI_ORIGIN_Y + hlRows[i] * UI_CELL,
                   UI_STONE_R + 4);
        }
        setlinewidth(1);
    }

    /* 右侧面板 */
    setfillcolor(COLOR_PANEL_BG);
    bar(UI_PANEL_X, 0, UI_WIDTH - 1, UI_HEIGHT - 1);

    set_text_font(30, COLOR_TEXT);
    outtextxy(x0, 26, "棋谱回放");
    set_text_font(16, COLOR_TEXT_DIM);
    outtextxy(x0, 66, "第15小组  程序设计实践");

    setcolor(COLOR_LINE);
    line(x0, 96, UI_WIDTH - 28, 96);

    /* 对局信息 */
    {
        int y = 112;
        set_text_font(19, COLOR_TEXT);

        if (info->mode == MODE_NET)
            sprintf(text, "模式：联网对战");
        else
            sprintf(text, "模式：%s", (info->mode == MODE_PVE) ? "人机对战" : "双人对战");
        outtextxy(x0, y, text);
        y += 28;

        if (info->mode == MODE_PVE)
        {
            sprintf(text, "难度：%s", ai_level_text(info->level));
            outtextxy(x0, y, text);
            y += 28;
            sprintf(text, "玩家执子：%s", (info->humanColor == CELL_BLACK) ? "黑棋" : "白棋");
            outtextxy(x0, y, text);
            y += 28;
        }

        sprintf(text, "总手数：%d", replayCount);
        outtextxy(x0, y, text);
        y += 28;

        set_text_font(17, COLOR_WARN);
        outtextxy(x0, y, status_line(board));
    }

    setcolor(COLOR_LINE);
    line(x0, 252, UI_WIDTH - 28, 252);

    /* 阵型解说：实时检测当前回放局面黑白双方的成型阵法 */
    {
        int fy = 262;
        FormationType blackF = ai_detect_formation(board, CELL_BLACK);
        FormationType whiteF = ai_detect_formation(board, CELL_WHITE);
        char fbuf[128];

        set_text_font(18, COLOR_ACCENT);
        outtextxy(x0, fy, "阵型解说");
        fy += 24;

        if (blackF == FORMATION_NONE && whiteF == FORMATION_NONE)
        {
            set_text_font(16, COLOR_TEXT_DIM);
            outtextxy(x0, fy, "暂无成型阵法");
        }
        else
        {
            if (blackF != FORMATION_NONE)
            {
                set_text_font(16, COLOR_TEXT);
                snprintf(fbuf, sizeof(fbuf), "黑方：%s", ai_formation_name(blackF));
                outtextxy(x0, fy, fbuf);
                fy += 22;
            }
            if (whiteF != FORMATION_NONE)
            {
                set_text_font(16, COLOR_TEXT);
                snprintf(fbuf, sizeof(fbuf), "白方：%s", ai_formation_name(whiteF));
                outtextxy(x0, fy, fbuf);
            }
        }
    }

    /* 进度条背景 */
    setfillcolor(COLOR_BTN);
    fill_round_rect(barX, barY, barW, barH, barH / 2);

    /* 进度条填充 */
    if (replayCount > 0)
    {
        fillW = barW * replayIndex / replayCount;
        if (fillW > barW) fillW = barW;
        if (fillW > 0)
        {
            setfillcolor(COLOR_ACCENT);
            if (fillW >= barH)
                fill_round_rect(barX, barY, fillW, barH, barH / 2);
            else
                bar(barX, barY, barX + fillW, barY + barH);
        }

        /* 当前位置滑块 */
        {
            int knobX = barX + fillW;
            int knobR = barH / 2 + 3;
            setfillcolor(COLOR_ACCENT);
            fillellipse(knobX, barY + barH / 2, knobR, knobR);
            setfillcolor(COLOR_TEXT);
            fillellipse(knobX, barY + barH / 2, knobR - 3, knobR - 3);
        }
    }

    /* 刻度线（25% / 50% / 75%） */
    setcolor(COLOR_LINE);
    setlinewidth(1);
    {
        int tick;
        for (tick = 1; tick < 4; tick++)
        {
            int tx = barX + barW * tick / 4;
            line(tx, barY + 2, tx, barY + barH - 2);
        }
    }

    /* 步数显示 */
    set_text_font(15, COLOR_TEXT);
    if (replayCount > 0)
        sprintf(text, "第 %d / %d 手", replayIndex, replayCount);
    else
        sprintf(text, "空棋谱");
    draw_text_center(barX + barW / 2, barY + barH + 14, text);

    /* 控制按钮 */
    for (i = 0; i < btnCount; i++)
    {
        int disabled = 0;
        if (replayButtons[i].id == BTN_REPLAY_FIRST ||
            replayButtons[i].id == BTN_REPLAY_PREV)
        {
            disabled = (replayIndex <= 0);
        }
        else if (replayButtons[i].id == BTN_REPLAY_NEXT ||
                 replayButtons[i].id == BTN_REPLAY_LAST)
        {
            disabled = (replayIndex >= replayCount);
        }

        if (disabled)
        {
            setfillcolor(COLOR_BTN);
            fill_round_rect(replayButtons[i].x, replayButtons[i].y,
                            replayButtons[i].w, replayButtons[i].h, 4);
            setcolor(COLOR_BTN_EDGE);
            setlinewidth(1);
            stroke_round_rect(replayButtons[i].x, replayButtons[i].y,
                              replayButtons[i].w, replayButtons[i].h, 4);
            set_text_font(15, COLOR_TEXT_DIM);
            draw_text_center(replayButtons[i].x + replayButtons[i].w / 2,
                              replayButtons[i].y + replayButtons[i].h / 2,
                              replayButtons[i].text);
        }
        else
        {
            draw_button(&replayButtons[i],
                        (replayButtons[i].id == hoverId), 0);
        }
    }
}

/* 每种风格的一句话定位(点击风格按钮时实时更新,第一行) */
static const char *style_title(int style)
{
    switch (style)
    {
    case STYLE_AGGRESSIVE: return "进攻系·猛攻 —— 攻势如潮";
    case STYLE_ATTACK:     return "进攻系·攻杀 —— 一击致命";
    case STYLE_DOUBLE:     return "进攻系·双杀 —— 双面夹击";
    case STYLE_WALL:       return "防守系·铁壁 —— 滴水不漏";
    case STYLE_COUNTER:    return "防守系·反击 —— 后发制人";
    case STYLE_TRICK:      return "战术系·技巧 —— 出人意料";
    case STYLE_CONTROL:    return "战术系·控场 —— 以势压人";
    case STYLE_BAIT:       return "战术系·诱导 —— 请君入瓮";
    case STYLE_NORMAL:
    default:               return "均衡系·普通 —— 攻守平衡";
    }
}

/* 每种风格的一句棋路特点(点击风格按钮时实时更新,第二行) */
static const char *style_detail(int style)
{
    switch (style)
    {
    case STYLE_AGGRESSIVE: return "进攻权重拉满，活三冲四优先，攻势凌厉但易被反击";
    case STYLE_ATTACK:     return "VCF 连杀优先，找到必胜杀路连续冲四，一击致命";
    case STYLE_DOUBLE:     return "一步双威胁（双活三/双冲四/活三+冲四），防不胜防";
    case STYLE_WALL:       return "防守权重拉满，对方活二必堵，滴水不漏，慢慢磨死对手";
    case STYLE_COUNTER:    return "前 10 手稳固防守，对手连攻 3 步后抓破绽突然反打";
    case STYLE_TRICK:      return "马步/八卦布局，落子出人意料，5% 概率随机变招";
    case STYLE_CONTROL:    return "中心掌控，势力范围压制，占据天元慢慢蚕食对手";
    case STYLE_BAIT:       return "前 8 手故意放活二诱你进攻，踏入陷阱区域后反杀";
    case STYLE_NORMAL:
    default:               return "攻守平衡，进攻防守权重均衡，标准最优策略";
    }
}

/*==============================================================================
 * 难度详情弹窗
 *============================================================================*/
static void diff_dialog_content(int level, const char **title,
                                 const char **tag,
                                 const char *lines[], int *lineCount,
                                 const char **optim, const char **styleHint)
{
    switch (level)
    {
    case LEVEL_BEGINNER:
        *title = "入门 · 随机算法";
        *tag   = "随机算法";
        lines[0] = "混沌初开，乾坤未定——棋盘上黑白二气自虚空而生。";
        lines[1] = "落子如星坠，全凭天意指引，无招胜有招。";
        lines[2] = "时而北地凝霜，时而南疆燃火，观者莫不称奇。";
        lines[3] = "此乃棋道之始：后续每一档算法，皆由此混沌中演化而出，";
        lines[4] = "先通晓评估之灵，再修搜索之功，终达剪枝之妙。";
        lines[5] = "进攻方式：无招可循，全凭天意，令对手无从预判。";
        lines[6] = "初学者可在此感受棋盘之无穷变化。";
        *lineCount = 7;
        *optim = "无";
        *styleHint = "本难度完全随机，无风格选项";
        break;
    case LEVEL_EASY:
        *title = "简单 · 贪心算法";
        *tag   = "贪心算法";
        lines[0] = "只看眼前一步：哪个空点最赚就走哪个。";
        lines[1] = "给每个点算分——自己的进攻分×2 + 对方的威胁分，";
        lines[2] = "选分数最高的落子。能认出活三、冲四这些关键棋型。";
        lines[3] = "但只盯一步，对手设的陷阱往往看不穿，";
        lines[4] = "这正是后面多步搜索要解决的问题。";
        lines[5] = "进攻方式：连出活三逼你防守，但不设连环杀招。";
        lines[6] = "适合新手熟悉棋型，感受“评估”的力量。";
        *lineCount = 7;
        *optim = "无";
        *styleHint = "本难度固定普通型，提升难度后解锁风格";
        break;
    case LEVEL_NORMAL:
        *title = "普通 · 极小极大（Minimax）";
        *tag   = "博弈搜索";
        lines[0] = "不再只看一步，而是预判你的下一步：";
        lines[1] = "假设双方都走最优——轮到我选对我最有利的，";
        lines[2] = "轮到你选对你最有利的，一攻一守来回推演 4 层，";
        lines[3] = "把整棵“对策树”完整算一遍，不做任何偷懒剪枝。";
        lines[4] = "能提前挡你的活三，也知道自己的活三别被堵死。";
        lines[5] = "进攻方式：四步连招，连续制造活三逼你防守。";
        lines[6] = "这是博弈搜索的第一课，高级搜索都由它进化而来。";
        *lineCount = 7;
        *optim = "无";
        *styleHint = "本难度固定普通型，提升难度后解锁风格";
        break;
    case LEVEL_HARD:
        *title = "困难 · Alpha-Beta 剪枝";
        *tag   = "剪枝搜索";
        lines[0] = "核心算法：α-β 剪枝。推演时必输分支直接剪掉，";
        lines[1] = "结果不变，速度提升数倍，推演深度加深到 6 层。";
        lines[2] = "配置换表（Zobrist）：已算局面缓存复用，避免重复搜索。";
        lines[3] = "能识别活三、冲四等关键棋型，会布置组合陷阱。";
        lines[4] = "进攻：活三+冲四连环施压，六步内逼你防守。";
        lines[5] = "棋力接近业余水平，是搜索引擎的标准技术。";
        lines[6] = "一句话：剪枝让搜索更快，置换表让搜索更省。";
        *lineCount = 7;
        *optim = "置换表（Zobrist Hash）";
        *styleHint = "解锁 普通 / 激进 / 铁壁 三风格";
        break;
    case LEVEL_MASTER:
        *title = "大师 · 迭代加深 IDDFS";
        *tag   = "迭代加深搜索";
        lines[0] = "核心算法：迭代加深。从 2 步起逐层加深搜索，";
        lines[1] = "历史启发＋杀手启发：上一层的好点，下一层优先算。";
        lines[2] = "限时 1.6 秒，时间越多想得越深，深算至 8 层。";
        lines[3] = "VCF 必杀检测：发现连续冲四能赢，直接走杀路。";
        lines[4] = "进攻：深算＋连杀一体，能长线布局一击致命。";
        lines[5] = "特点：会算与会杀结合，攻防转换成熟。";
        lines[6] = "一句话：迭代加深让搜索更聪明，VCF 让杀棋更准。";
        *lineCount = 7;
        *optim = "置换表 + 历史启发 + 杀手启发 + VCF";
        *styleHint = "在困难档基础上新增 反击 / 控场 / 双杀";
        break;
    case LEVEL_GRANDMASTER:
        *title = "棋圣 · 深算 IDDFS + VCF";
        *tag   = "深算搜索";
        lines[0] = "核心算法：迭代加深（IDDFS）＋VCF 必胜检测，六档天花板。";
        lines[1] = "VCF 连杀检测：发现连续冲四必胜杀路，直接一击制胜。";
        lines[2] = "2400ms 专属深算预算，逐层加深至 10~12 层，远超大师。";
        lines[3] = "开局库加持：前几手强占花月/浦月要点，下限极高。";
        lines[4] = "进攻：深算与连杀一体，职业级攻杀路线。";
        lines[5] = "特点：算得最深、杀得最准，人类难以逾越的棋力上限。";
        lines[6] = "一句话：把时间全砸进深算，让 VCF 把优势兑现成胜局。";
        *lineCount = 7;
        *optim = "置换表 + 历史启发 + 杀手启发 + VCF + 强开局库";
        *styleHint = "在大师档基础上新增 技巧 / 诱导 / 攻杀，共九种风格";
        break;
    default:
        *title = "未知";
        *tag   = "";
        lines[0] = "";
        *lineCount = 1;
        *optim = "";
        *styleHint = "";
        break;
    }
}

/* 获取当前难度可用的风格按钮列表（含坐标），返回按钮数量。
 * 绘制与命中检测共用此函数，避免两处维护坐标。
 * 布局参数与 ui_draw_diff_dialog 弹窗一致：dx=180, dy=60。 */
int ui_get_style_buttons(int level, UiButton *buttons, int maxButtons)
{
    static const UiButton styleDefs[9] =
    {
        { 0, 0, 110, 28, BTN_STYLE_NORMAL,     "均衡·普通" },
        { 0, 0, 110, 28, BTN_STYLE_AGGRESSIVE, "进攻·猛攻" },
        { 0, 0, 110, 28, BTN_STYLE_WALL,       "防守·铁壁" },
        { 0, 0, 110, 28, BTN_STYLE_COUNTER,    "防守·反击" },
        { 0, 0, 110, 28, BTN_STYLE_CONTROL,    "战术·控场" },
        { 0, 0, 110, 28, BTN_STYLE_DOUBLE,     "进攻·双杀" },
        { 0, 0, 110, 28, BTN_STYLE_TRICK,      "战术·技巧" },
        { 0, 0, 110, 28, BTN_STYLE_BAIT,       "战术·诱导" },
        { 0, 0, 110, 28, BTN_STYLE_ATTACK,     "进攻·攻杀" }
    };
    const int btnX0     = 180 + 44;    /* dx + 44 */
    const int btnColGap = 130;
    const int btnRowGap = 32;
    const int styleY0   = 60 + 502;     /* dy + 502 */
    int styleRows = 0;
    int count = 0;
    int row;
    int col;

    if (level >= LEVEL_HARD)        styleRows = 1;
    if (level >= LEVEL_MASTER)      styleRows = 2;
    if (level >= LEVEL_GRANDMASTER) styleRows = 3;

    for (row = 0; row < styleRows; row++)
    {
        for (col = 0; col < 3; col++)
        {
            if (count >= maxButtons) return count;
            buttons[count] = styleDefs[row * 3 + col];
            buttons[count].x = btnX0 + col * btnColGap;
            buttons[count].y = styleY0 + row * btnRowGap;
            count++;
        }
    }
    return count;
}

void ui_draw_diff_dialog(int level, int style, int hoverId)
{
    static const UiButton buttons[] =
    {
        { 330, 668, 180, 40, BTN_DIFF_CONFIRM, "确认选择" },
        { 650, 668, 180, 40, BTN_DIFF_CANCEL,  "取消"     }
    };
    const int btnCount = 2;
    const int dx = 180;
    const int dy = 60;
    const int dw = 820;
    const int dh = 680;
    const char *title;
    const char *tag;
    const char *lines[8];
    const char *optim;
    const char *styleHint;
    int  lineCount;
    int  styleRows;    /* 0=固定风格 1=困难 2=大师 3=棋圣 */
    int  i;
    int  tagW;

    diff_dialog_content(level, &title, &tag, lines, &lineCount, &optim, &styleHint);

    /* 遮罩层 */
    setfillcolor(EGERGB(20, 20, 30));
    bar(0, 0, UI_WIDTH - 1, UI_HEIGHT - 1);

    /* 面板背景 */
    setfillcolor(COLOR_PANEL_BG);
    fill_round_rect(dx, dy, dw, dh, 14);
    setcolor(COLOR_ACCENT);
    setlinewidth(2);
    stroke_round_rect(dx, dy, dw, dh, 14);

    /* 标题 */
    set_text_font(34, COLOR_ACCENT);
    draw_text_center(dx + dw / 2, dy + 40, title);

    /* 算法类型标签 */
    tagW = (int)strlen(tag) * 17 + 26;
    setfillcolor(COLOR_ACCENT);
    fill_round_rect(dx + dw / 2 - tagW / 2, dy + 84, tagW, 28, 14);
    set_text_font(17, COLOR_PANEL_BG);
    draw_text_center(dx + dw / 2, dy + 84 + 14, tag);

    /* 分隔线 */
    setcolor(COLOR_LINE);
    setlinewidth(1);
    line(dx + 44, dy + 124, dx + dw - 44, dy + 124);

    /* 算法原理小节 */
    set_text_font(20, COLOR_ACCENT);
    outtextxy(dx + 44, dy + 138, "【算法原理】");

    /* 介绍文字(23 号,行距 34) */
    set_text_font(23, COLOR_TEXT);
    for (i = 0; i < lineCount; i++)
    {
        outtextxy(dx + 48, dy + 166 + i * 34, lines[i]);
    }

    /* 优化模块行 */
    set_text_font(20, COLOR_TEXT_DIM);
    outtextxy(dx + 44, dy + 418, "优化模块:");
    set_text_font(20, COLOR_TEXT);
    outtextxy(dx + 152, dy + 418, optim);

    /* 分隔线 */
    setcolor(COLOR_LINE);
    line(dx + 44, dy + 448, dx + dw - 44, dy + 448);

    /* 下棋风格小节 */
    set_text_font(20, COLOR_ACCENT);
    outtextxy(dx + 44, dy + 462, "【下棋风格】");
    set_text_font(18, COLOR_TEXT_DIM);
    outtextxy(dx + 190, dy + 468, styleHint);

    styleRows = 0;
    if (level >= LEVEL_HARD)        styleRows = 1;
    if (level >= LEVEL_MASTER)      styleRows = 2;
    if (level >= LEVEL_GRANDMASTER) styleRows = 3;

    if (styleRows == 0)
    {
        /* 低难度:固定风格,不显示任何风格按钮 */
        set_text_font(19, COLOR_TEXT_DIM);
        draw_text_center(dx + dw / 2, dy + 520,
                         (level == LEVEL_BEGINNER)
                             ? "本难度为完全随机落子，无风格选项"
                             : "本难度固定普通型风格；选择困难及以上难度可解锁风格");
    }
    else
    {
        /* 风格按钮布局统一由 ui_get_style_buttons 提供（绘制与命中检测共用） */
        UiButton styleButtons[9];
        int styleCount = ui_get_style_buttons(level, styleButtons, 9);
        const int infoX    = dx + 440;
        const int styleY0  = dy + 502;
        int si;
        for (si = 0; si < styleCount; si++)
        {
            int sValue;
            switch (styleButtons[si].id)
            {
            case BTN_STYLE_NORMAL:     sValue = STYLE_NORMAL;     break;
            case BTN_STYLE_AGGRESSIVE: sValue = STYLE_AGGRESSIVE; break;
            case BTN_STYLE_WALL:       sValue = STYLE_WALL;       break;
            case BTN_STYLE_COUNTER:    sValue = STYLE_COUNTER;    break;
            case BTN_STYLE_CONTROL:    sValue = STYLE_CONTROL;    break;
            case BTN_STYLE_DOUBLE:     sValue = STYLE_DOUBLE;     break;
            case BTN_STYLE_TRICK:      sValue = STYLE_TRICK;      break;
            case BTN_STYLE_BAIT:       sValue = STYLE_BAIT;       break;
            default:                   sValue = STYLE_ATTACK;     break;
            }
            draw_style_button(&styleButtons[si], (styleButtons[si].id == hoverId),
                              (style == sValue));
        }
        /* 选中风格简介:竖排在按钮右侧,标题高亮 + 详情换行最多 3 行 */
        set_text_font(20, COLOR_ACCENT);
        outtextxy(infoX, styleY0, style_title(style));
        set_text_font(18, COLOR_TEXT_DIM);
        draw_text_wrapped(infoX, styleY0 + 34, dx + dw - 44 - infoX, 27, 3,
                          style_detail(style));
    }

    /* 按钮 */
    for (i = 0; i < btnCount; i++)
    {
        draw_button(&buttons[i], (buttons[i].id == hoverId), 0);
    }
}

/*==============================================================================
 * 兼容旧接口（保留以防外部调用）
 *============================================================================*/
int ui_input_line(const char *title, const char *prompt, char *buf, int size)
{
    if (buf == NULL || size <= 0) return 0;
    return (inputbox_getline(title, prompt, buf, size) != 0) ? 1 : 0;
}

/*==============================================================================
 * 算法百科页（产品介绍 / AI 发展历程 + 核心代码逐行讲解）
 * 由历史背景引入，由浅及深介绍每一档难度：先背景、再原理、最后真实代码逐行讲解。
 *============================================================================*/
typedef struct
{
    const char *tag;        /* 章节标签 */
    const char *meta;       /* 时代/里程碑副信息 */
    const char *headline;   /* 页面主标题 */
    const char *body[12];   /* 正文行（NULL 结尾；可含空行） */
    const char *code[18];   /* 核心代码行（NULL 结尾；无代码页首行为 NULL） */
    const char *expl[18];   /* 与 code 逐行对应的通俗讲解 */
} AlgoPage;

static const AlgoPage kAlgoPages[] =
{
    /* ---------------- 第 0 页 · 总览 ---------------- */
    {
        "序 · 一条算法进化史",
        "六档难度 × AI 发展历程",
        "从随机到棋圣：一部浓缩的 AI 简史",
        {
            "人工智能从诞生至今走过 60 余年。巧合的是，本游戏的六档难度恰好串起了这条历史主线——每一档都对应 AI 棋类程序的一个重要里程碑：",
            "",
            "    1956  达特茅斯会议（AI 元年）   →  入门 · 随机算法",
            "    1959  Samuel 跳棋程序          →  简单 · 贪心算法",
            "    1944  冯·诺依曼博弈论          →  普通 · 极小极大",
            "    1960s  Alpha-Beta 剪枝          →  困难 · Alpha-Beta",
            "    1997  深蓝战胜人类棋王         →  大师 · 迭代加深",
            "    2016  AlphaGo 击败李世石       →  棋圣 · 深算 IDDFS+VCF",
            "",
            "跟着这条时间轴翻页：每页先讲历史背景，再讲算法原理，最后把五子棋里的真实代码逐行拆给你看。",
            NULL
        },
        { NULL }
    },

    /* ---------------- 第 1 页 · 入门（历史背景） ---------------- */
    {
        "① 入门 · 随机算法",
        "1956 · 达特茅斯会议：AI 诞生元年",
        "历史背景：混沌初开，一切从随机开始",
        {
            "1956 年夏，达特茅斯学院的一场研讨会第一次提出『人工智能』一词，被视为 AI 的诞生元年。",
            "当时的程序谈不上智能：既不会『看』棋盘，也不会『想』对策，只能按最朴素的规则运转。",
            "随机落子，就是最原始的状态——它把一切判断都交给概率，当作所有算法的起点与对照基线。",
            "",
            "【算法原理】",
            "随机算法不做任何判断：先找出所有『有邻居』的空位（紧贴已有棋子的地方），",
            "再用 rand() 随机挑一个落子。它唯一的规则是『下在棋盘上且不重复』，既无进攻、也无防守。",
            "放进来是为了让玩家直观感受：如果没有评估、没有搜索，机器下棋会是怎样的『混沌』模样。",
            NULL
        },
        { NULL }
    },

    /* ---------------- 第 2 页 · 入门（代码） ---------------- */
    {
        "① 入门 · 随机算法",
        "核心代码：random_choose（src/ai_search.c）",
        "五子棋里如何实现『随机』",
        {
            "全部智能只有三行：收集候选 → 随机挑一个 → 返回。",
            "注意 collect_empty_neighbors 只保留『有邻居』的空位，避免 AI 把子下到偏远角落。",
            NULL
        },
        {
            "int random_choose(const int grid[15][15],",
            "                  int *bestRow, int *bestCol)",
            "{",
            "    int cellList[MAX_MOVES];   /* 候选空位表 */",
            "    int count, pick;",
            "    count = collect_empty_neighbors(grid, cellList);",
            "    if (count == 0) return AI_ERR_NO_MOVE;",
            "    pick = rand() % count;      /* 随机挑一个 */",
            "    *bestRow = cellList[pick] / 15;  /* 还原行 */",
            "    *bestCol = cellList[pick] % 15;  /* 还原列 */",
            "    return AI_OK;",
            NULL
        },
        {
            "函数入口：接收棋盘，落子坐标通过指针传出",
            "",
            "",
            "用一个数组装下所有『可落子』的空位",
            "",
            "只收集有邻居的空点（贴近棋子的位置）",
            "棋盘满了、无处可下 → 返回错误码",
            "rand() 对候选数取模 → 等概率随机选中一个",
            "候选点用一维编号存，÷15 还原成行号",
            "÷15 取余还原成列号——入门档的全部『智能』",
            "返回成功，由上层执行落子",
            NULL
        }
    },

    /* ---------------- 第 3 页 · 简单（历史背景） ---------------- */
    {
        "② 简单 · 贪心算法",
        "1959 · Samuel 跳棋程序：第一个会『打分』的棋类程序",
        "历史背景：机器第一次学会『打分』",
        {
            "1959 年，Arthur Samuel 写出了著名的跳棋程序——公认的第一个『会评估局面』的棋类程序：它给每个局面打分、选分数高的走，标志着 AI 从随机迈入『启发式』时代。",
            "贪心算法正是评估思想的最简形态：只看眼前一步，哪个空点『得分』最高就走哪个。",
            "",
            "【算法原理】",
            "为每个候选空点打分：自己的进攻分 ×2 + 对方的威胁分（对方若在此落子能成五/活三，威胁极大）。",
            "取分数最高的落子；并列最高则随机打破平局，避免永远走同一手。",
            "局限：只看一步，看不见对手设下的连环陷阱——这正是下一页『多步搜索』要解决的问题。",
            NULL
        },
        { NULL }
    },

    /* ---------------- 第 4 页 · 简单（代码） ---------------- */
    {
        "② 简单 · 贪心算法",
        "核心代码：greedy_choose（src/ai_search.c）",
        "五子棋里如何实现『贪心』",
        {
            "gen_candidates 会为每个候选点算好 score（进攻分×2 + 威胁分）。",
            "贪心只做一件事：遍历一遍，记住分数最高的那个点。",
            NULL
        },
        {
            "int greedy_choose(const int grid[15][15], int me,",
            "                  int width, int *bestRow, int *bestCol)",
            "{",
            "    AiCandidate candidate[AI_MAX_CANDIDATE];",
            "    int count, best, i;",
            "    count = gen_candidates(grid, me, candidate, width);",
            "    if (count == 0) return AI_ERR_NO_MOVE;",
            "    best = 0;",
            "    for (i = 1; i < count; i++)          /* 找最高分 */",
            "        if (candidate[i].score > candidate[best].score)",
            "            best = i;                    /* 记住最高分点 */",
            "    *bestRow = candidate[best].row;      /* 取最高分落子 */",
            "    *bestCol = candidate[best].col;",
            "    return AI_OK;",
            NULL
        },
        {
            "入口：棋盘、本方颜色、候选宽度与输出指针",
            "",
            "",
            "候选点数组：每个点带 row/col/score",
            "",
            "为所有候选点打分（进攻×2 + 威胁×1）",
            "无处可下 → 返回错误码",
            "先假设第 0 个候选是最高分",
            "遍历剩余候选，找真正最高分",
            "发现更高的就更新 best 下标",
            "（并列最高停在第一个）",
            "把最高分点的坐标写回输出",
            "",
            "返回成功",
            NULL
        }
    },

    /* ---------------- 第 5 页 · 普通（历史背景） ---------------- */
    {
        "③ 普通 · 极小极大",
        "1944 · 博弈论奠基：『我一步，你一步』的数学化",
        "历史背景：AI 开始『推演未来』",
        {
            "1944 年，冯·诺依曼与摩根斯坦合著《博弈论与经济行为》，为『零和博弈』奠定了数学基础。",
            "核心洞察：五子棋是零和游戏——我赢你就输。因此可以推演：我走一步，假设你也走最优，我再走……",
            "极小极大（Minimax）正是把这种推演变成算法：轮到我时取『对我最有利』的分值（极大），轮到你时取『对你最有利、即对我最不利』的分值（极小）。",
            "",
            "【算法原理】",
            "从当前局面展开一棵『博弈树』：节点是一次落子，叶子是评估分。我方节点取子节点最大值，对方节点取最小值，层层回传，得到当前最优着法。",
            "本游戏普通档完整展开 4 层博弈树、不做任何剪枝——虽慢，却最直观地展示了『搜索』的本质。",
            NULL
        },
        { NULL }
    },

    /* ---------------- 第 6 页 · 普通（代码） ---------------- */
    {
        "③ 普通 · 极小极大",
        "核心代码：minimax_search（src/ai_search.c）",
        "五子棋里如何实现『推演』",
        {
            "核心是递归：落一子 → 换对手 → 深度减一，直到叶节点用评估函数打分。",
            "轮到我取最大分，轮到对手取最小分——这就是『极小极大』名字的由来。",
            NULL
        },
        {
            "int minimax_search(int grid[15][15], int depth, int me,",
            "                   int turn, int width, int *bestRow,",
            "                   int *bestCol)",
            "{",
            "    if (depth <= 0) return evaluate_grid(grid, me);",
            "    count = gen_candidates(grid, turn, candidate, width);",
            "    if (turn == me) {              /* 该我走：取最大 */",
            "        best = -AI_INFINITY;",
            "        for (i = 0; i < count; i++) {",
            "            grid[row][col] = turn;           /* 模拟落子 */",
            "            value = minimax_search(grid, depth-1, me,",
            "                                   OPPONENT(turn), NULL, NULL);",
            "            grid[row][col] = CELL_EMPTY;     /* 撤销落子 */",
            "            if (value > best) best = value;  /* 取最大 */",
            "        }",
            "    }",
            "    return best;   /* 对手回合对称：取最小（极小） */",
            NULL
        },
        {
            "入口：棋盘、剩余深度、本方颜色",
            "",
            "",
            "",
            "深度耗尽 → 返回局面评估分（不再往下想）",
            "生成当前回合的候选点",
            "分支1：轮到本方走 → 目标是『最大』",
            "下界初始为负无穷",
            "遍历所有候选落子",
            "在棋盘上临时落子",
            "递归搜索下一层：换对手、深度减一",
            "（每试一步立即撤销，恢复原局面）",
            "撤销落子",
            "记录本分支中的最大值",
            "",
            "",
            "对手回合的目标是『最小』，代码对称",
            NULL
        }
    },

    /* ---------------- 第 7 页 · 困难（历史背景） ---------------- */
    {
        "④ 困难 · Alpha-Beta 剪枝",
        "1960s · 剪枝技术：让搜索第一次『跑得动』",
        "历史背景：聪明地跳过无用分支",
        {
            "极小极大要展开整棵博弈树，分支数随深度指数爆炸，深度一大就跑不动。",
            "1960 年代，Alpha-Beta 剪枝技术成熟，让博弈搜索第一次真正实用：搜索中发现某个分支『已经不可能比已知结果更好了』，就立即剪掉——结果完全不变，计算量却大幅下降。",
            "这是后来 IBM 深蓝、各类棋类引擎的核心技术。",
            "",
            "【算法原理】",
            "维护两个值：α（我方已知的最优下界）、β（对方已知的最优上界）。",
            "搜索中一旦 α ≥ β，说明这条线双方都不会再选，立即停止展开该分支（剪枝）。",
            "本游戏困难档用 Alpha-Beta 深推 6 层，并叠加 Zobrist 置换表缓存局面，棋力接近业余水平。",
            NULL
        },
        { NULL }
    },

    /* ---------------- 第 8 页 · 困难（代码） ---------------- */
    {
        "④ 困难 · Alpha-Beta 剪枝",
        "核心代码：alphabeta_search（src/ai_search.c）",
        "五子棋里如何实现『剪枝』",
        {
            "与极小极大结构几乎一样，只在两处多了一行：更新 α/β，一旦 α ≥ β 立即 break。",
            "剪掉的都是『确定不会比现有结果更好』的分支，所以结果一字不差、速度成倍提升。",
            NULL
        },
        {
            "int alphabeta_search(int grid[15][15], int depth, int alpha,",
            "                     int beta, int me, int turn, int width,",
            "                     U64 hash, int *bestRow, int *bestCol)",
            "{",
            "    if (depth <= 0) return evaluate_grid(grid, me);",
            "    if (turn == me) {              /* 我方节点：取最大 */",
            "        best = -AI_INFINITY;",
            "        for (i = 0; i < count; i++) {",
            "            grid[row][col] = turn;           /* 模拟落子 */",
            "            value = alphabeta_search(grid, depth-1,",
            "                                     alpha, beta, ...);",
            "            grid[row][col] = CELL_EMPTY;     /* 撤销 */",
            "            if (best > alpha) alpha = best;  /* 更新下界α */",
            "            if (alpha >= beta) break;        /* ★剪枝核心 */",
            "        }",
            "    }",
            "    return best;   /* 对手节点对称：维护上界β */",
            NULL
        },
        {
            "入口：多出 alpha/beta 两个窗口参数",
            "",
            "",
            "",
            "同样的递归骨架，评估函数打分不变",
            "我方节点：目标是『最大』",
            "下界初始为负无穷",
            "遍历候选落子",
            "模拟落子",
            "递归搜索，把 α、β 传下去（窗口收窄）",
            "",
            "撤销落子",
            "发现更优的，就抬高下界 α",
            "α ≥ β → 这分支不可能被选，立即剪枝",
            "",
            "",
            "对手节点对称维护上界 β",
            NULL
        }
    },

    /* ---------------- 第 9 页 · 大师（历史背景） ---------------- */
    {
        "⑤ 大师 · 迭代加深",
        "1997 · 深蓝：计算机第一次战胜人类棋王",
        "历史背景：限时之下，『一层层想』",
        {
            "1997 年，IBM 深蓝击败国际象棋世界冠军卡斯帕罗夫，震惊世界。它每步评估约 2 亿个局面，制胜法宝之一就是迭代加深 + 海量优化。",
            "现实对局必须限时（本游戏每步限时 1.6 秒）。问题来了：一次性搜到底，超时了怎么办？",
            "迭代加深（IDDFS）的答案：从浅层开始一层层加深，每层先用上一层的结果排序；时间一到，就用『当前已算完的最深层』结果，绝不留白。",
            "",
            "【算法原理】",
            "深度 2 → 4 → 6 → 8…逐层递增，每层完整搜一遍，上一层的好点优先进入下一层的排序（历史启发、杀手启发）。",
            "再叠加 VCF 必胜检测：发现连续冲四能赢就直接走杀路，形成『会算又会杀』的棋力。",
            NULL
        },
        { NULL }
    },

    /* ---------------- 第 10 页 · 大师（代码） ---------------- */
    {
        "⑤ 大师 · 迭代加深",
        "核心代码：iddfs_loop（src/ai_search.c）",
        "五子棋里如何实现『一层层想』",
        {
            "外层是一个逐层加深的循环，内层直接复用 Alpha-Beta 搜索。",
            "超时就用『上一层已完成』的结果——这就是『时间换深度』的优雅之处。",
            NULL
        },
        {
            "int iddfs_loop(int grid[15][15], int me, int maxDepth,",
            "               int width, U64 hash, int *bestRow,",
            "               int *bestCol)",
            "{",
            "    count = gen_candidates(grid, me, candidate, width);",
            "    *bestRow = candidate[0].row;  /* 保底候选 */",
            "    for (i = 2; i <= maxDepth; i += 2)   /* 2→4→6→… */",
            "    {",
            "        alphabeta_search(grid, i, -AI_INFINITY,",
            "                         AI_INFINITY, me, me, width,",
            "                         hash, &depthRow, &depthCol);",
            "        if (s_timeout) break;     /* 超时立即收手 */",
            "        if (depthRow >= 0)",
            "            *bestRow = depthRow;  /* 记住本层最佳 */",
            "        if (clock() > s_deadline - 半预算) break;",
            "    }",
            "    return AI_OK;   /* 返回已算完的最深层结果 */",
            NULL
        },
        {
            "入口：maxDepth 是棋圣档的 12、大师档的 8",
            "",
            "",
            "先生成候选，若为空直接失败",
            "先记一个保底候选（最坏也能落子）",
            "深度从 2 起、每次 +2（偶数层视角一致）",
            "",
            "内层直接调用 Alpha-Beta 搜到第 i 层",
            "",
            "",
            "一旦超时就立即 break，绝不硬撑",
            "本层若搜出了更优解",
            "就用本层的结果覆盖（逐层逼近更优）",
            "",
            "预算过半提前收手，给上层留保底时间",
            "",
            "最终返回『已算完的最深层』的结果",
            NULL
        }
    },

    /* ---------------- 第 11 页 · 棋圣（历史背景） ---------------- */
    {
        "⑥ 棋圣 · 深算 IDDFS + VCF",
        "2016 · AlphaGo：AI 棋力全面登顶",
        "历史背景：深算 + 必胜，人类难以逾越",
        {
            "2016 年，AlphaGo 4:1 击败围棋世界冠军李世石，宣告 AI 在策略博弈上全面超越人类。",
            "顶级棋力靠两件事：一是『算得深』，二是『认得准』——一眼看出『这条线我必胜』。",
            "棋圣档正是这两点的实现：VCF 必胜检测（连续冲四的必杀路线，直接一击制胜）+ 迭代加深深算（2400ms 专属预算，深至 10~12 层，配合强开局库）。",
            "",
            "【算法原理】",
            "VCF（Victory by Continuous Four）：若能连续冲四形成必胜杀路，直接执行、不再深算。",
            "无必胜时，用 IDDFS 深算到 10~12 层。自对弈实测：棋圣对大师，分出胜负局胜率 80%，是本游戏的棋力天花板。",
            NULL
        },
        { NULL }
    },

    /* ---------------- 第 12 页 · 棋圣（代码） ---------------- */
    {
        "⑥ 棋圣 · 深算 IDDFS + VCF",
        "核心代码：棋圣档决策 + detect_vcf（src/ai.c、ai_vcf.c）",
        "五子棋里如何实现『棋圣』",
        {
            "棋圣的决策只有两步：先问『有没有必胜杀路』，没有就深算。",
            "detect_vcf 在棋盘副本上递归寻找连续冲四，找到就立即返回杀路、直接赢下。",
            NULL
        },
        {
            "case LEVEL_GRANDMASTER:           /* 棋圣档入口 */",
            "    /* 第一步：VCF 必胜检测 */",
            "    if (detect_vcf(grid, player, &bestRow, &bestCol) == 1)",
            "        break;                      /* 有杀路→直接赢 */",
            "    /* 第二步：深算 12 层 */",
            "    ret = iddfs_loop(grid, player, 12, width,",
            "                     hash, &bestRow, &bestCol);",
            "    break;",
            "",
            "/* detect_vcf：在副本棋盘上探索必胜 */",
            "int detect_vcf(const int grid[15][15], int me,",
            "               int *outRow, int *outCol)",
            "{",
            "    memcpy(g, grid, sizeof(g));   /* 副本，不污染局面 */",
            "    return vcf_search(g, me, VCF_MAX_DEPTH, outRow, outCol);",
            "    /* vcf_search：递归找连续冲四，每手都必须是冲四，",
            "       连成五子即必胜——找到就直接走杀路 */",
            NULL
        },
        {
            "棋圣档的分支入口（src/ai.c）",
            "先做 VCF 必胜检测",
            "找到连续冲四必胜杀路 → 返回 1",
            "有杀路就直接落子，跳过后续深算",
            "没有必胜，才进入深算",
            "深算 12 层，2400ms 专属预算",
            "",
            "深算完成，输出落子",
            "",
            "detect_vcf 入口（src/ai_vcf.c）",
            "",
            "",
            "",
            "在副本 g 上探索，不动真实棋盘",
            "调用递归 VCF 搜索，限最深 VCF_MAX_DEPTH",
            "连续冲四直到连成五子，即『必胜』",
            "",
            NULL
        }
    },

    /* ---------------- 第 13 页 · 结语 ---------------- */
    {
        "结语 · 从随机到棋圣",
        "第15小组 · 程序设计实践 · 2026 暑期小学期",
        "一条路，就是一部 AI 简史",
        {
            "随机（1956）→ 贪心（1959）→ 极小极大（1944）→ Alpha-Beta（1960s）→ 迭代加深（1997）→ 深算（2016）",
            "",
            "每一档都建立在前一档之上：评估是搜索的基础，剪枝让搜索变快，迭代加深让『快』变得可控，必胜检测让『强』变得致命。",
            "",
            "技术栈：C（GCC 4.9.2）+ EGE 20.08 图形库 · 七模块结构 · Winsock 局域网对战",
            "六档难度、九种风格、阵法解说、自对弈验证——欢迎回到主菜单，亲自体验每一档棋力！",
            "",
            "（使用下方『上一页 / 下一页』翻页，『返回主菜单』退出）",
            NULL
        },
        { NULL }
    }
};

#define ALGO_PAGE_COUNT ((int)(sizeof(kAlgoPages) / sizeof(kAlgoPages[0])))

int ui_algo_page_count(void)
{
    return ALGO_PAGE_COUNT;
}

/* 算法百科页翻页按钮（绘制与命中检测共用，避免两处维护坐标） */
int ui_get_algo_buttons(int page, UiButton *buttons, int maxButtons)
{
    int n = 0;
    int pageCount = ALGO_PAGE_COUNT;

    if (buttons == NULL || maxButtons < 3) return 0;
    if (page > 0)
        buttons[n++] = (UiButton) { 430, 752, 140, 38, BTN_ALGO_PREV, "← 上一页" };
    if (page < pageCount - 1)
        buttons[n++] = (UiButton) { 580, 752, 140, 38, BTN_ALGO_NEXT, "下一页 →" };
    buttons[n++] = (UiButton) { 880, 752, 200, 38, BTN_ALGO_BACK, "返回主菜单" };
    return n;
}

/* 算法百科页绘制（历史背景 + 算法原理 + 核心代码逐行讲解） */
void ui_draw_algo_book(int page, int hoverId)
{
    const AlgoPage *p;
    UiButton buttons[3];
    int btnCount;
    int i;

    if (page < 0 || page >= ALGO_PAGE_COUNT) page = 0;
    p = &kAlgoPages[page];

    draw_gradient_bg(COLOR_MENU_BG1, COLOR_MENU_BG2);

    /* 页眉 */
    set_text_font(20, COLOR_TEXT);
    outtextxy(60, 22, "算法百科 · AI 进阶之路");
    {
        char pageNo[32];
        sprintf(pageNo, "第 %d / %d 页", page + 1, ALGO_PAGE_COUNT);
        set_text_font(16, COLOR_TEXT_DIM);
        outtextxy(UI_WIDTH - 60 - textwidth(pageNo), 26, pageNo);
    }

    /* 章节标签 + 时代副信息 */
    set_text_font(22, COLOR_ACCENT);
    outtextxy(60, 62, p->tag);
    if (p->meta != NULL && p->meta[0] != '\0')
    {
        set_text_font(16, COLOR_TEXT_DIM);
        outtextxy(60, 94, p->meta);
    }

    /* 主标题 */
    set_text_font(26, COLOR_TEXT);
    outtextxy(60, 124, p->headline);

    /* 分隔线 */
    setcolor(COLOR_ACCENT);
    setlinewidth(2);
    line(60, 152, UI_WIDTH - 60, 152);
    setlinewidth(1);

    /* 正文区（有代码页最多 4 行，无代码页最多 13 行） */
    {
        int maxBody = (p->code[0] != NULL) ? 4 : 13;
        int y = 168;
        for (i = 0; i < maxBody && p->body[i] != NULL; i++)
        {
            set_text_font(19, COLOR_TEXT);
            draw_text_wrapped(60, y, UI_WIDTH - 120, 32, 1, p->body[i]);
            y += 32;
        }
    }

    /* 代码区（逐行讲解） */
    if (p->code[0] != NULL)
    {
        int y = 316;
        set_text_font(18, COLOR_ACCENT);
        draw_text_center(UI_WIDTH / 2, 288, "——— 核心代码 · 逐行讲解 ———");

        for (i = 0; i < 18 && p->code[i] != NULL; i++)
        {
            int codeW;
            set_text_font(17, EGERGB(0x9F, 0xE2, 0xC0));
            codeW = textwidth(p->code[i]);
            outtextxy(60, y, p->code[i]);

            if (p->expl[i] != NULL && p->expl[i][0] != '\0')
            {
                int explX = 60 + codeW + 14;
                /* 注释紧跟代码右侧，消除原 x=620 的大空档；过宽则换到下一行 */
                if (explX + textwidth(p->expl[i]) > UI_WIDTH - 60)
                {
                    set_text_font(15, COLOR_TEXT_DIM);
                    outtextxy(60, y + 20, p->expl[i]);
                    y += 22;
                }
                else
                {
                    set_text_font(16, COLOR_TEXT_DIM);
                    outtextxy(explX, y + 1, p->expl[i]);
                }
            }
            y += 26;
            if (y > 736) break;
        }
    }

    /* 底部翻页按钮 */
    btnCount = ui_get_algo_buttons(page, buttons, 3);
    for (i = 0; i < btnCount; i++)
        draw_button(&buttons[i], (buttons[i].id == hoverId), 0);
}
