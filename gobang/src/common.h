/*==============================================================================
 * 文件名称：common.h
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：公共定义（全体成员共用）
 * 功能描述：定义棋盘规格、棋子取值、对局模式、难度等级、对局状态等全局常量，
 *           以及描述一局棋对局设置的 GameInfo 结构体。
 * 编码说明：源文件以 UTF-8 保存，编译时用 -fexec-charset=GBK 转为 GBK。
 * 创建日期：2026-08-25
 *============================================================================*/
#ifndef GOBANG_COMMON_H
#define GOBANG_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------ 棋盘规格 ------------------------------*/
#define BOARD_SIZE      15
#define WIN_COUNT       5
#define MAX_MOVES       (BOARD_SIZE * BOARD_SIZE)

/*------------ 棋盘格取值 -------------*/
#define CELL_EMPTY      0
#define CELL_BLACK      1
#define CELL_WHITE      2

/*------------------------------ 对局模式 ------------------------------*/
#define MODE_PVP        0
#define MODE_PVE        1
#define MODE_NET        2

/*---------------------------- AI 难度等级 -----------------------------*/
#define LEVEL_BEGINNER      0
#define LEVEL_EASY          1
#define LEVEL_NORMAL        2
#define LEVEL_HARD          3
#define LEVEL_MASTER        4   /* 大师:迭代加深搜索(IDDFS) */
#define LEVEL_GRANDMASTER   5   /* 棋圣:PVS 主要变例搜索(P2 落地) */
#define LEVEL_COUNT         6

/*---------------------------- AI 下棋风格 -----------------------------*/
#define STYLE_NORMAL     0   /* 普通型:攻守平衡(所有难度) */
#define STYLE_AGGRESSIVE 1   /* 激进型:进攻 ×1.6 / 防守 ×0.5(困难+) */
#define STYLE_ATTACK     2   /* 攻杀型:VCF/VCT 连招优先(P2) */
#define STYLE_DOUBLE     3   /* 双杀型:一步双威胁(P1) */
#define STYLE_WALL       4   /* 铁壁型:进攻 ×0.5 / 防守 ×1.6(困难+) */
#define STYLE_COUNTER    5   /* 反击型:先守后攻(P1) */
#define STYLE_TRICK      6   /* 技巧型:马步/八卦布局(P2) */
#define STYLE_CONTROL    7   /* 控场型:中心掌控(P1) */
#define STYLE_BAIT       8   /* 诱导型:陷阱反杀(P2) */
#define STYLE_COUNT      9

/*------------------------------ 对局状态 ------------------------------*/
#define GAME_PLAYING    0
#define GAME_BLACK_WIN  1
#define GAME_WHITE_WIN  2
#define GAME_DRAW       3

#define OPPONENT(player)  ((player) == CELL_BLACK ? CELL_WHITE : CELL_BLACK)

typedef struct
{
    int mode;
    int level;
    int humanColor;
    int style;          /* AI 下棋风格(STYLE_*) */
} GameInfo;

/* ---- 统一文本函数（各模块共用，避免重复定义） ---- */
const char *common_mode_text(int mode);
const char *common_level_text(int level);
const char *common_status_text(int status);
const char *common_style_text(int style);
const char *common_color_text(int color);

#ifdef __cplusplus
}
#endif

#endif /* GOBANG_COMMON_H */
