/*==============================================================================
 * 文件名称：ai_internal.h
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：AI 引擎模块（内部共享头文件）
 * 功能描述：声明 AI 各子模块（eval/search/vcf/formation）之间共享的
 *           类型、全局变量和内部函数。对外接口仍在 ai.h 中声明。
 * 创建日期：2026-08-26
 *============================================================================*/
#ifndef GOBANG_AI_INTERNAL_H
#define GOBANG_AI_INTERNAL_H

#include "ai.h"
#include <time.h>

/*==============================================================================
 * 常量与类型
 *============================================================================*/

#define AI_INFINITY       100000000
#define AI_WIN_SCORE       10000000
#define AI_MAX_CANDIDATE         32
#define AI_NEIGHBOR_RANGE         2
#define AI_TIME_BUDGET_MS      1600
#define AI_TIME_BUDGET_MS_GM   2400   /* 棋圣专属：深算时间预算，拉开与大师的深度差距 */
#define AI_MAX_DEPTH             16

/* 置换表（Zobrist Hash） */
typedef unsigned long long U64;
#define TT_SIZE  (1 << 14)
#define TT_MASK  (TT_SIZE - 1)
#define TT_EXACT 0
#define TT_LOWER 1
#define TT_UPPER 2

typedef struct
{
    U64  hash;
    int  depth;
    int  flag;
    int  score;
    int  bestRow;
    int  bestCol;
} TtEntry;

/* 候选点 */
typedef struct
{
    int row;
    int col;
    int score;
} AiCandidate;

/* 四方向常量（定义在 ai.c） */
extern const int AI_DIR[DIR_COUNT][2];

/*==============================================================================
 * 全局状态变量（定义在 ai.c，各子模块共享）
 *============================================================================*/

/* 搜索时间控制 */
extern clock_t s_deadline;
extern int     s_budgetMs;      /* 当前决策的实际时间预算（棋圣档加大） */
extern int     s_timeout;
extern int     s_lastDepth;
extern int     s_lastTimeMs;

/* 风格系数（由 ai_set_style 设置） */
extern int s_style;
extern int s_attackW;
extern int s_defendW;
extern int s_candAtkMul;
extern int s_candDefMul;
extern int s_candCenterW;
extern int s_candAreaW;
extern int s_candKnightW;
extern int s_candFormW;

/* 移动排序启发（大师档及以上） */
extern int s_history[BOARD_SIZE][BOARD_SIZE];
extern int s_killer[AI_MAX_DEPTH][2];
extern int s_useHistory;
extern int s_useKiller;

/* 置换表 */
extern TtEntry s_tt[TT_SIZE];
extern U64     s_zobrist[3][BOARD_SIZE][BOARD_SIZE];
extern U64     s_rng;
extern int     s_ttOn;

/* 空着裁剪（可选演示） */
extern int s_nullMoveOn;

/*==============================================================================
 * 工具函数（ai.c）
 *============================================================================*/
U64 rng_next(void);
int in_range(int row, int col);

/*==============================================================================
 * 评估函数（ai_eval.c）
 *============================================================================*/
int pattern_score(int len, int blocked);
int point_score(const int grid[BOARD_SIZE][BOARD_SIZE], int row, int col, int player);
int has_five(const int grid[BOARD_SIZE][BOARD_SIZE], int row, int col, int player);
int evaluate_side(const int grid[BOARD_SIZE][BOARD_SIZE], int player);
int live_threat_bonus(const int grid[BOARD_SIZE][BOARD_SIZE], int me);
int evaluate_grid(const int grid[BOARD_SIZE][BOARD_SIZE], int me);

/*==============================================================================
 * 候选生成（ai_eval.c）
 *============================================================================*/
int has_neighbor(const int grid[BOARD_SIZE][BOARD_SIZE], int row, int col);
int collect_empty_neighbors(const int grid[BOARD_SIZE][BOARD_SIZE], int *out);
int center_bonus(int row, int col);
int area_bonus(const int grid[BOARD_SIZE][BOARD_SIZE], int row, int col, int me);
int knight_bonus(const int grid[BOARD_SIZE][BOARD_SIZE], int row, int col, int me);
int threat_count_at(const int grid[BOARD_SIZE][BOARD_SIZE], int row, int col, int me);
int gen_candidates(const int grid[BOARD_SIZE][BOARD_SIZE], int turn,
                   AiCandidate *out, int maxOut);

/*==============================================================================
 * 移动排序（ai_search.c）
 *============================================================================*/
void killer_store(int depth, int row, int col);
void sort_by_killer(AiCandidate *cand, int count, int depth);
void sort_by_history(AiCandidate *cand, int count);

/*==============================================================================
 * VCF / VCT 杀棋检测（ai_vcf.c）
 *============================================================================*/
int collect_five_points(const int grid[BOARD_SIZE][BOARD_SIZE], int me,
                        int *rows, int *cols, int cap);
int gen_si_points(const int grid[BOARD_SIZE][BOARD_SIZE], int me,
                  AiCandidate *out, int maxOut);
int vcf_search(int grid[BOARD_SIZE][BOARD_SIZE], int me, int depth,
               int *outRow, int *outCol);
int detect_vcf(const int grid[BOARD_SIZE][BOARD_SIZE], int me,
               int *outRow, int *outCol);
int gen_threat_points(const int grid[BOARD_SIZE][BOARD_SIZE], int me,
                      AiCandidate *out, int maxOut);
int open_three_ends(const int grid[BOARD_SIZE][BOARD_SIZE],
                    int row, int col, int me,
                    int *endsR, int *endsC, int cap);
int vct_search(int grid[BOARD_SIZE][BOARD_SIZE], int me, int depth,
               int *outRow, int *outCol);
int detect_vct(const int grid[BOARD_SIZE][BOARD_SIZE], int me,
               int *outRow, int *outCol);

/*==============================================================================
 * 置换表操作（ai_search.c）
 *============================================================================*/
void    tt_clear(void);
TtEntry *tt_probe(U64 hash);
void    tt_store(U64 hash, int depth, int flag, int score, int bestRow, int bestCol);
U64     board_hash(const int grid[BOARD_SIZE][BOARD_SIZE]);

/*==============================================================================
 * 搜索算法（ai_search.c）
 *============================================================================*/
int alphabeta_search(int grid[BOARD_SIZE][BOARD_SIZE], int depth,
                     int alpha, int beta, int me, int turn, int width,
                     U64 hash, int *bestRow, int *bestCol);
int pvs_search(int grid[BOARD_SIZE][BOARD_SIZE], int depth,
               int alpha, int beta, int me, int turn, int width,
               U64 hash, int *bestRow, int *bestCol);
int minimax_search(int grid[BOARD_SIZE][BOARD_SIZE], int depth,
                   int me, int turn, int width, int *bestRow, int *bestCol);
int random_choose(const int grid[BOARD_SIZE][BOARD_SIZE], int *bestRow, int *bestCol);
int greedy_choose(const int grid[BOARD_SIZE][BOARD_SIZE], int me, int width,
                  int *bestRow, int *bestCol);
int iddfs_loop(int grid[BOARD_SIZE][BOARD_SIZE], int me, int maxDepth, int width,
                U64 hash, int *bestRow, int *bestCol);
int iddfs_pvs_loop(int grid[BOARD_SIZE][BOARD_SIZE], int me, int maxDepth, int width,
                    U64 hash, int *bestRow, int *bestCol);
void level_config(int level, int *maxDepth, int *width);

/*==============================================================================
 * 风格触发检测（ai.c）
 *============================================================================*/
int opponent_in_trap(const Board *board, int dist);
int opponent_recent_attack(const Board *board, int steps);

/*==============================================================================
 * 开局库（ai_formation.c）
 *============================================================================*/
int apply_opening(const int grid[BOARD_SIZE][BOARD_SIZE], int player,
                  int moveCount, int *row, int *col);

/*==============================================================================
 * 阵型识别（ai_formation.c）
 *============================================================================*/
int           detect_meihua(const int grid[BOARD_SIZE][BOARD_SIZE], int me);
int           count_bagua_pairs(const int grid[BOARD_SIZE][BOARD_SIZE], int me);
int           has_long_diag(const int grid[BOARD_SIZE][BOARD_SIZE], int me, int *nDiag);
int           detect_liuxing(const int grid[BOARD_SIZE][BOARD_SIZE], int me);
int           detect_xiesan(const int grid[BOARD_SIZE][BOARD_SIZE], int me);
int           detect_sijiao(const int grid[BOARD_SIZE][BOARD_SIZE], int me);
int           detect_shizi(const int grid[BOARD_SIZE][BOARD_SIZE], int me);
FormationType formation_from_grid(const int grid[BOARD_SIZE][BOARD_SIZE], int player);
int           formation_hint_grid(const int grid[BOARD_SIZE][BOARD_SIZE], int player);

#endif /* GOBANG_AI_INTERNAL_H */
