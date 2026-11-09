/*==============================================================================
 * 文件名称：ui.h
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：界面模块              负责人：雷子豪
 * 功能描述：声明窗口初始化、菜单绘制、棋盘绘制、状态提示绘制等接口。
 *============================================================================*/
#ifndef GOBANG_UI_H
#define GOBANG_UI_H

#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UI_WIDTH      1180
#define UI_HEIGHT     800
#define UI_CELL        40
#define UI_ORIGIN_X    40
#define UI_ORIGIN_Y    90
#define UI_STONE_R     17
#define UI_PANEL_X    760

#define BTN_NONE          (-1)
#define BTN_PVP             1
#define BTN_PVE             2
#define BTN_LEVEL_EASY      3
#define BTN_LEVEL_NORMAL    4
#define BTN_LEVEL_HARD      5
#define BTN_LEVEL_BEGINNER  16
#define BTN_LEVEL_MASTER    17
#define BTN_LEVEL_GRANDMASTER 27
#define BTN_STYLE_NORMAL    28
#define BTN_STYLE_AGGRESSIVE 29
#define BTN_STYLE_WALL      30
#define BTN_STYLE_COUNTER   31
#define BTN_STYLE_CONTROL   32
#define BTN_STYLE_DOUBLE    33
#define BTN_STYLE_TRICK     34
#define BTN_STYLE_BAIT      35
#define BTN_STYLE_ATTACK    36
#define BTN_DIFF_CONFIRM    18
#define BTN_DIFF_CANCEL     19
#define BTN_COLOR_BLACK     6
#define BTN_COLOR_WHITE     7
#define BTN_MENU_LOAD       8
#define BTN_MENU_EXIT       9
#define BTN_NET_HOST       10
#define BTN_NET_JOIN       11
#define BTN_NET_CANCEL     12
#define BTN_NET_CONNECT    13
#define BTN_JOIN_BACK      14
#define BTN_THEME          15
#define BTN_UNDO           20
#define BTN_RESIGN         21
#define BTN_SAVE           22
#define BTN_LOAD           23
#define BTN_RESTART        24
#define BTN_BACK           25
#define BTN_LOAD_BACK      26
#define BTN_ROW_BASE       40
#define BTN_REPLAY_FIRST   50
#define BTN_REPLAY_PREV    51
#define BTN_REPLAY_NEXT    52
#define BTN_REPLAY_LAST    53
#define BTN_REPLAY_BACK    54
#define BTN_ALGO_BOOK      60
#define BTN_ALGO_PREV      61
#define BTN_ALGO_NEXT      62
#define BTN_ALGO_BACK      63

/* 主题定义 */
#define THEME_CLASSIC  0
#define THEME_DARK     1
#define THEME_OCEAN    2
#define THEME_JADE     3
#define THEME_VIOLET   4
#define THEME_SUNSET   5
#define THEME_COUNT    6

typedef struct
{
    int         x;
    int         y;
    int         w;
    int         h;
    int         id;
    const char *text;
} UiButton;

typedef struct
{
    int         y;
    int         id;
    const char *text;
    const char *subtext;
} UiRow;

void ui_init(void);
void ui_close(void);
int ui_menu_buttons(const UiButton **buttons);
int ui_game_buttons(const UiButton **buttons);
void ui_draw_menu(const GameInfo *info, int hoverId);
void ui_draw_game(const Board *board, const GameInfo *info,
                  const char *message, int hoverId,
                  int hoverRow, int hoverCol, int thinking,
                  const int *hlRows, const int *hlCols, int hlCount);
void ui_flush(void);
void ui_draw_browser(const UiRow *rows, int rowCount, int hoverId,
                     const char *message);
void ui_draw_net_wait(int isHost, const char *status, const char *localIp,
                      const char *targetIp, int hoverId);
void ui_draw_net_join(const char *ip, int cursorVisible, int hoverId,
                      const char *message);
void ui_draw_replay(const Board *board, const GameInfo *info,
                    int replayIndex, int replayCount, int hoverId,
                    const int *hlRows, const int *hlCols, int hlCount);
void ui_draw_diff_dialog(int level, int style, int hoverId);
/* 算法百科页（产品介绍 / AI 发展历程 + 核心代码逐行讲解） */
int  ui_algo_page_count(void);
int  ui_get_algo_buttons(int page, UiButton *buttons, int maxButtons);
void ui_draw_algo_book(int page, int hoverId);
/* 获取当前难度可用的风格按钮列表（含坐标），返回按钮数量。
 * 绘制与命中检测共用此函数，避免两处维护坐标。 */
int  ui_get_style_buttons(int level, UiButton *buttons, int maxButtons);
void ui_set_theme(int theme);
int  ui_get_theme(void);
const char *ui_theme_name(int theme);
/* 阵型播报:推送一条解说进右侧面板滚动区(保留最近 3 条,text 为空则清空) */
void ui_set_formation_report(const char *text);
/* 设置当前阵型事件文案（仅在成型那一步显示，传NULL清空） */
void ui_set_formation_event(const char *text);
int ui_input_line(const char *title, const char *prompt, char *buf, int size);

#ifdef __cplusplus
}
#endif

#endif /* GOBANG_UI_H */
