/*==============================================================================
 * 文件名称：ai.h
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：AI 引擎模块            负责人：chang814
 * 功能描述：声明人机对战的落点决策接口与局面评估接口。
 *============================================================================*/
#ifndef GOBANG_AI_H
#define GOBANG_AI_H

#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AI_OK          0
#define AI_ERR_NULL  (-1)
#define AI_ERR_NO_MOVE (-2)

#define AI_SCORE_FIVE        100000
#define AI_SCORE_OPEN_FOUR    10000
#define AI_SCORE_FOUR          5000
#define AI_SCORE_OPEN_THREE    3000
#define AI_SCORE_SLEEP_THREE    500
#define AI_SCORE_OPEN_TWO       200
#define AI_SCORE_SLEEP_TWO       50
#define AI_SCORE_ONE             10

void ai_init(unsigned int seed);
void ai_set_style(int style);
const char *ai_style_text(int style);
void ai_tt_enable(int on);   /* 置换表开关(调试/对照实验) */
void ai_set_null_move(int on); /* 空着裁剪开关(默认关,答辩演示) */
int ai_mcts_choose(const Board *board, int player, int simsPerMove,
                   int *row, int *col);   /* 蒙特卡洛模拟选点(可选演示) */
int ai_choose(const Board *board, int player, int level, int *row, int *col);
int ai_point_score(const Board *board, int row, int col, int player);
int ai_evaluate(const Board *board, int player);
int ai_last_depth(void);
int ai_last_time_ms(void);
const char *ai_level_text(int level);

/* ---- 阵型识别与解说(第4.2节:棋盘旁播报 + 网红阵法解说) ---- */

/* 阵法类型枚举:先实现可稳定识别的民间基础阵法,后续按资料库扩展 */
typedef enum
{
    FORMATION_NONE     = 0,   /* 未成型 */
    FORMATION_MEIHUA,         /* 梅花阵:中心子 + 4 斜对角 ≥3 */
    FORMATION_BAGUA,          /* 八卦阵:全盘马步(日字)对 ≥5 */
    FORMATION_YAN,            /* 燕阵:两翼斜线对称展开 */
    FORMATION_LIUXING,        /* 流星阵:同一条斜线 ≥4 子连续进攻 */
    FORMATION_XIESAN,         /* 斜三阵:三子斜连成角 */
    FORMATION_SIJIAO,         /* 四角阵:四子成正方形 */
    FORMATION_SHIZI,          /* 十字阵:四子成十字 */
    FORMATION_COUNT
} FormationType;

/* 形状匹配 + 胜率验证,返回成型阵法(同一步可能命中多个,取优先级最高者) */
FormationType ai_detect_formation(const Board *board, int player);
/* 生成激情解说文本(含阵法名 + 一句话简介 + 棋路特点) */
void ai_formation_comment(const Board *board, int player,
                          char *buf, int size);
/* 技巧型候选加分:当前 player 可摆成阵法的候选点位数量(0..N) */
int  ai_formation_hint(const Board *board, int player);
/* 阵法中文名(供播报显示) */
const char *ai_formation_name(FormationType f);
/* 获取当前 player 成型阵法的棋子坐标，返回棋子数量（0表示无成型阵法） */
int ai_formation_cells(const Board *board, int player, int *rows, int *cols, int maxCells);
/* 生成阵型事件文案：根据手数自动区分开局/中盘风格 */
void ai_formation_event_text(const Board *board, int player, char *buf, int size);

#ifdef __cplusplus
}
#endif

#endif /* GOBANG_AI_H */
