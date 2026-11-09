/*==============================================================================
 * 文件名称：ai.c
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：AI 引擎模块 — 主入口与全局状态
 * 功能描述：定义 AI 全局状态变量、四方向常量、随机数生成器，
 *           以及对外接口（ai_init / ai_choose / ai_set_style / ai_evaluate 等）。
 *           评估、搜索、VCF/VCT、阵型识别等子模块分别在 ai_eval.c /
 *           ai_search.c / ai_vcf.c / ai_formation.c 中实现。
 * 创建日期：2026-08-25（v3.1 重构拆分于 2026-08-26）
 *============================================================================*/
#include "ai_internal.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

/*==============================================================================
 * 四方向常量（横/竖/主斜/副斜）
 *============================================================================*/
const int AI_DIR[DIR_COUNT][2] =
{
    { 0,  1 },
    { 1,  0 },
    { 1,  1 },
    { 1, -1 }
};

/*==============================================================================
 * 全局状态变量定义（各子模块通过 ai_internal.h 中的 extern 声明共享）
 *============================================================================*/

/* 搜索时间控制 */
clock_t s_deadline    = 0;
int     s_budgetMs    = AI_TIME_BUDGET_MS;
int     s_timeout     = 0;
int     s_lastDepth   = 0;
int     s_lastTimeMs  = 0;

/* 风格系数（由 ai_set_style 设置） */
int s_style      = STYLE_NORMAL;
int s_attackW    = 10;   /* 己方进攻权重（10 = 1.0） */
int s_defendW    = 11;   /* 对方威胁防守权重（默认 11/10 防守偏重） */
int s_candAtkMul = 2;    /* 候选排序：己方进攻分倍率 */
int s_candDefMul = 1;    /* 候选排序：对方防守分倍率 */
int s_candCenterW = 0;   /* 中心控制权加分权重 */
int s_candAreaW   = 0;   /* 势力范围延伸加分权重 */
int s_candKnightW = 0;   /* 技巧型：马步（日字）加分权重 */
int s_candFormW   = 0;   /* 技巧型：阵法候选点加分权重 */

/* 移动排序启发（大师档及以上） */
int s_history[BOARD_SIZE][BOARD_SIZE];   /* 历史启发：导致剪枝的走法加分 */
int s_killer[AI_MAX_DEPTH][2];           /* 杀手启发：每层 2 个剪枝走法 */
int s_useHistory = 0;
int s_useKiller  = 0;

/* 置换表（Zobrist Hash） */
TtEntry s_tt[TT_SIZE];
U64     s_zobrist[3][BOARD_SIZE][BOARD_SIZE];   /* 索引 0 不用 */
U64     s_rng;
int     s_ttOn = 1;     /* 置换表开关（调试/对照实验用） */

/* 空着裁剪（可选演示） */
int s_nullMoveOn = 0;

/*==============================================================================
 * 工具函数
 *============================================================================*/

/* 64 位伪随机源（xorshift64，用于生成 Zobrist 随机表） */
U64 rng_next(void)
{
    s_rng ^= s_rng << 13;
    s_rng ^= s_rng >> 7;
    s_rng ^= s_rng << 17;
    return s_rng;
}

int in_range(int row, int col)
{
    return (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE);
}

/*==============================================================================
 * 风格触发检测（反击型/诱导型专用）
 *============================================================================*/

/* 诱导型触发检测：对手最后一手是否进入己方核心区外沿（陷阱区域） */
int opponent_in_trap(const Board *board, int dist)
{
    int me  = board->current;
    int cnt = 0;
    int dr;
    int dc;
    int r;
    int c;

    if (board->moveCount == 0) return 0;
    r = board->moves[board->moveCount - 1].row;
    c = board->moves[board->moveCount - 1].col;

    for (dr = -dist; dr <= dist; dr++)
    {
        for (dc = -dist; dc <= dist; dc++)
        {
            int rr = r + dr;
            int cc = c + dc;
            if (in_range(rr, cc) && board->grid[rr][cc] == me) cnt++;
        }
    }
    return cnt >= 2;
}

/* 反击型触发检测：对手是否最近连续 steps 步均走出活三/冲四级威胁 */
int opponent_recent_attack(const Board *board, int steps)
{
    int count = 0;
    int i;

    for (i = board->moveCount - 1; i >= 0; i--)
    {
        const Move *mv = &board->moves[i];
        if (mv->player == board->current) break;   /* 已方步：连续序列结束 */
        if (point_score(board->grid, mv->row, mv->col, mv->player) >= AI_SCORE_OPEN_THREE)
            count++;
        else
            break;
        if (count >= steps) return 1;
    }
    return 0;
}

/*==============================================================================
 * 对外接口：初始化
 *============================================================================*/

void ai_init(unsigned int seed)
{
    int stone;
    int row;
    int col;

    srand(seed);
    s_rng = (U64)seed + 0x9E3779B97F4A7C15ULL;

    /* 生成 Zobrist 随机表：每格×每子一个 64 位随机数 */
    for (stone = 1; stone <= 2; stone++)
    {
        for (row = 0; row < BOARD_SIZE; row++)
        {
            for (col = 0; col < BOARD_SIZE; col++)
            {
                s_zobrist[stone][row][col] = rng_next();
            }
        }
    }

    s_lastDepth  = 0;
    s_lastTimeMs = 0;
    ai_set_style(STYLE_NORMAL);

    /* 置换表在初始化时清空一次，对局中跨手数复用（Zobrist 哈希编码完整局面，
     * 不同手数的局面哈希不同，不会冲突）。新对局开始时由 main 调用 ai_init 重置。 */
    tt_clear();
}

/*==============================================================================
 * 对外接口：评估
 *============================================================================*/

int ai_point_score(const Board *board, int row, int col, int player)
{
    if (board == NULL || !in_range(row, col)) return 0;
    return point_score(board->grid, row, col, player);
}

int ai_evaluate(const Board *board, int player)
{
    if (board == NULL) return 0;
    if (player != CELL_BLACK && player != CELL_WHITE) return 0;
    return evaluate_grid(board->grid, player);
}

/*==============================================================================
 * 对外接口：主决策入口
 *============================================================================*/

int ai_choose(const Board *board, int player, int level, int *row, int *col)
{
    int          grid[BOARD_SIZE][BOARD_SIZE];
    AiCandidate  candidate[AI_MAX_CANDIDATE];
    int          count;
    int          maxDepth;
    int          width;
    int          i;
    int          bestRow = -1;
    int          bestCol = -1;
    int          opponent;
    int          ret;
    U64          hash;
    clock_t      startTime;

    if (board == NULL || row == NULL || col == NULL) return AI_ERR_NULL;
    if (player != CELL_BLACK && player != CELL_WHITE) return AI_ERR_NULL;
    if (board_is_full(board)) return AI_ERR_NO_MOVE;

    opponent    = OPPONENT(player);
    startTime   = clock();
    s_budgetMs  = (level == LEVEL_GRANDMASTER) ? AI_TIME_BUDGET_MS_GM : AI_TIME_BUDGET_MS;
    s_deadline  = startTime + (clock_t)(s_budgetMs * CLOCKS_PER_SEC / 1000);
    s_timeout   = 0;
    s_lastDepth = 0;

    memcpy(grid, board->grid, sizeof(grid));

    /* 置换表跨手数复用：不再每次决策前清空。
     * Zobrist 哈希编码了完整局面（每格每子），不同手数的局面哈希不同，
     * 查表时深度不足的条目会被忽略（tt_probe 要求 e->depth >= depth）。
     * 新对局开始时 ai_init 会清空一次置换表。 */
    hash = board_hash(grid);

    /* 启发表每次决策前清空；大师档及以上启用历史/杀手启发 */
    memset(s_history, 0, sizeof(s_history));
    memset(s_killer, 0, sizeof(s_killer));
    s_useHistory = (level >= LEVEL_MASTER);
    s_useKiller  = (level >= LEVEL_MASTER);

    /* 反击型：按局面阶段动态切换攻防权重
     * 前 10 手防守 ×1.4；对手连续 ≥3 步活三/冲四后转反攻 ×1.4 */
    if (s_style == STYLE_COUNTER)
    {
        if (opponent_recent_attack(board, 3))
        {
            s_attackW = 14;
            s_defendW = 11;
        }
        else if (board->moveCount < 20)
        {
            s_attackW = 10;
            s_defendW = 15;
        }
        else
        {
            s_attackW = 10;
            s_defendW = 11;
        }
    }

    /* 诱导型：前 8 手故意放活二（防守权重极低），对手进入陷阱区域后猛攻 */
    if (s_style == STYLE_BAIT)
    {
        if (opponent_in_trap(board, 2))
        {
            s_attackW = 16;
            s_defendW = 5;
        }
        else if (board->moveCount <= 14)
        {
            s_attackW = 10;
            s_defendW = 5;                 /* 故意不堵非关键活二 */
        }
        else
        {
            s_attackW = 10;
            s_defendW = 11;
        }
    }

    if (board->moveCount == 0)
    {
        *row = BOARD_SIZE / 2;
        *col = BOARD_SIZE / 2;
        s_lastTimeMs = 0;
        return AI_OK;
    }

    level_config(level, &maxDepth, &width);

    /* 开局库：棋圣档前几手用固定强开局提高下限（黑第3手/白第2手） */
    if (level >= LEVEL_GRANDMASTER &&
        apply_opening(grid, player, board->moveCount, &bestRow, &bestCol))
    {
        *row = bestRow;
        *col = bestCol;
        s_lastTimeMs = 0;
        return AI_OK;
    }

    count = gen_candidates(grid, player, candidate, width);
    if (count == 0) return AI_ERR_NO_MOVE;

    /* 我方一步成五：直接赢 */
    for (i = 0; i < count; i++)
    {
        if (point_score(grid, candidate[i].row, candidate[i].col, player) >= AI_SCORE_FIVE)
        {
            *row = candidate[i].row;
            *col = candidate[i].col;
            s_lastTimeMs = (int)((clock() - startTime) * 1000 / CLOCKS_PER_SEC);
            return AI_OK;
        }
    }

    /* 对方一步成五：必须防守 */
    for (i = 0; i < count; i++)
    {
        if (point_score(grid, candidate[i].row, candidate[i].col, opponent) >= AI_SCORE_FIVE)
        {
            *row = candidate[i].row;
            *col = candidate[i].col;
            s_lastTimeMs = (int)((clock() - startTime) * 1000 / CLOCKS_PER_SEC);
            return AI_OK;
        }
    }

    /* 按难度档位调度核心算法 */
    switch (level)
    {
    case LEVEL_BEGINNER:
        ret = random_choose(grid, &bestRow, &bestCol);
        break;
    case LEVEL_EASY:
        ret = greedy_choose(grid, player, width, &bestRow, &bestCol);
        break;
    case LEVEL_NORMAL:
        minimax_search(grid, 4, player, player, width, &bestRow, &bestCol);
        ret = (bestRow >= 0) ? AI_OK : AI_ERR_NO_MOVE;
        if (ret == AI_OK) s_lastDepth = 4;
        break;
    case LEVEL_HARD:
        alphabeta_search(grid, 6, -AI_INFINITY, AI_INFINITY,
                         player, player, width, hash, &bestRow, &bestCol);
        ret = (bestRow >= 0) ? AI_OK : AI_ERR_NO_MOVE;
        if (ret == AI_OK) s_lastDepth = 6;
        break;
    case LEVEL_MASTER:
        /* VCF 必胜检测：找到连续冲四杀路直接执行 */
        if (detect_vcf(grid, player, &bestRow, &bestCol) == 1)
        {
            ret = AI_OK;
            break;
        }
        ret = iddfs_loop(grid, player, 8, width, hash, &bestRow, &bestCol);
        break;
    case LEVEL_GRANDMASTER:
        /* 棋圣：VCF → 深度迭代加深（Alpha-Beta 主干，专属长时预算内加深到 12 层）。
         * 说明（2026-08-26 自对弈回归）：VCT 活三链检测常出现「伪必胜」——
         * 在对手存在更深反杀时仍判定己方必胜，一旦命中就直接落子且完全跳过
         * 深度搜索，导致棋圣下出未经验证的激进棋，实测反而打不过大师
         * （12%~40%）。故从自动路径摘除 VCT（函数保留），棋圣=VCF+更深搜索，
         * 并配 2400ms 专属深算预算（AI_TIME_BUDGET_MS_GM），使棋圣>大师梯度恢复。 */
        if (detect_vcf(grid, player, &bestRow, &bestCol) == 1)
        {
            ret = AI_OK;
            break;
        }
        ret = iddfs_loop(grid, player, 12, width, hash, &bestRow, &bestCol);
        break;
    default:
        ret = iddfs_loop(grid, player, 2, width, hash, &bestRow, &bestCol);
        break;
    }
    if (ret != AI_OK) return ret;

    *row = bestRow;
    *col = bestCol;
    s_lastTimeMs = (int)((clock() - startTime) * 1000 / CLOCKS_PER_SEC);

    return AI_OK;
}

/*==============================================================================
 * 对外接口：搜索信息查询
 *============================================================================*/

int ai_last_depth(void)
{
    return s_lastDepth;
}

int ai_last_time_ms(void)
{
    return s_lastTimeMs;
}

const char *ai_level_text(int level)
{
    switch (level)
    {
    case LEVEL_BEGINNER:    return "入门";
    case LEVEL_EASY:        return "简单";
    case LEVEL_HARD:        return "困难";
    case LEVEL_MASTER:      return "大师";
    case LEVEL_GRANDMASTER: return "棋圣";
    case LEVEL_NORMAL:      return "普通";
    default:                return "未知";
    }
}

/*==============================================================================
 * 对外接口：风格与调试开关
 *============================================================================*/

void ai_tt_enable(int on)
{
    s_ttOn = (on != 0);
}

void ai_set_null_move(int on)
{
    s_nullMoveOn = (on != 0);
}

/* 风格 = 评估函数权重（attack/defend）+ 候选排序倍率 + 布局型加分权重 */
void ai_set_style(int style)
{
    switch (style)
    {
    case STYLE_AGGRESSIVE:                /* 激进型：进攻 ×1.6 / 防守 ×0.5，进攻点置顶 */
        s_attackW    = 16;
        s_defendW    = 5;
        s_candAtkMul = 3;
        s_candDefMul = 1;
        s_candCenterW = 0;
        s_candAreaW   = 0;
        s_candKnightW = 0;
        break;
    case STYLE_WALL:                      /* 铁壁型：进攻 ×0.5 / 防守 ×1.6，对方威胁置顶 */
        s_attackW    = 5;
        s_defendW    = 16;
        s_candAtkMul = 1;
        s_candDefMul = 2;
        s_candCenterW = 0;
        s_candAreaW   = 0;
        s_candKnightW = 0;
        break;
    case STYLE_COUNTER:                   /* 反击型：先守后攻，权重在 ai_choose 内按局面动态切换 */
        s_attackW    = 10;
        s_defendW    = 11;
        s_candAtkMul = 2;
        s_candDefMul = 1;
        s_candCenterW = 0;
        s_candAreaW   = 0;
        s_candKnightW = 0;
        break;
    case STYLE_CONTROL:                   /* 控场型：中心控制权 + 势力范围延伸加分 */
        s_attackW    = 10;
        s_defendW    = 11;
        s_candAtkMul = 2;
        s_candDefMul = 1;
        s_candCenterW = 50;
        s_candAreaW   = 20;
        s_candKnightW = 0;
        break;
    case STYLE_DOUBLE:                    /* 双杀型：一步双威胁点 ×3（gen_candidates 内检测） */
        s_attackW    = 10;
        s_defendW    = 11;
        s_candAtkMul = 2;
        s_candDefMul = 1;
        s_candCenterW = 0;
        s_candAreaW   = 0;
        s_candKnightW = 0;
        break;
    case STYLE_ATTACK:                    /* 攻杀型：VCF 连杀优先，权重极偏进攻 */
        s_attackW    = 18;
        s_defendW    = 5;
        s_candAtkMul = 3;
        s_candDefMul = 1;
        s_candCenterW = 0;
        s_candAreaW   = 0;
        s_candKnightW = 0;
        break;
    case STYLE_TRICK:                     /* 技巧型：马步/八卦布局 + 5% 随机扰动 */
        s_attackW    = 10;
        s_defendW    = 11;
        s_candAtkMul = 2;
        s_candDefMul = 1;
        s_candCenterW = 0;
        s_candAreaW   = 0;
        s_candKnightW = 30;
        s_candFormW   = 20;
        break;
    case STYLE_BAIT:                      /* 诱导型：前 8 手放活二，对手入陷阱后猛攻（ai_choose 内动态） */
        s_attackW    = 10;
        s_defendW    = 11;
        s_candAtkMul = 2;
        s_candDefMul = 1;
        s_candCenterW = 0;
        s_candAreaW   = 0;
        s_candKnightW = 0;
        break;
    case STYLE_NORMAL:
    default:                              /* 普通型：攻守平衡 */
        s_attackW    = 10;
        s_defendW    = 11;
        s_candAtkMul = 2;
        s_candDefMul = 1;
        s_candCenterW = 0;
        s_candAreaW   = 0;
        s_candKnightW = 0;
        break;
    }
    s_style = style;
}

const char *ai_style_text(int style)
{
    switch (style)
    {
    case STYLE_NORMAL:     return "普通型";
    case STYLE_AGGRESSIVE: return "激进型";
    case STYLE_ATTACK:     return "攻杀型";
    case STYLE_DOUBLE:     return "双杀型";
    case STYLE_WALL:       return "铁壁型";
    case STYLE_COUNTER:    return "反击型";
    case STYLE_TRICK:      return "技巧型";
    case STYLE_CONTROL:    return "控场型";
    case STYLE_BAIT:       return "诱导型";
    default:               return "未知";
    }
}

/*==============================================================================
 * 蒙特卡洛模拟选点（可选演示，不接入档位）
 * Flat Monte Carlo：对每个高优先级候选点做 N 次随机走子模拟，取胜率最高者。
 * 这是 MCTS（蒙特卡洛树搜索，AlphaGo 核心组件之一）的简化概念演示版：
 * 省略了 UCT 树选择/反向传播，仅保留"大量随机模拟 + 统计"的核心思想。
 * 棋力弱于 PVS，供答辩讲解蒙特卡洛方法时现场演示。
 *============================================================================*/
int ai_mcts_choose(const Board *board, int player, int simsPerMove,
                   int *row, int *col)
{
    AiCandidate cand[AI_MAX_CANDIDATE];
    int grid[BOARD_SIZE][BOARD_SIZE];
    int count;
    int i;
    int sim;
    int bestSim = -1;

    if (board == NULL || row == NULL || col == NULL) return AI_ERR_NULL;
    if (player != CELL_BLACK && player != CELL_WHITE) return AI_ERR_NULL;
    if (board_is_full(board)) return AI_ERR_NO_MOVE;
    if (simsPerMove <= 0) simsPerMove = 100;

    memcpy(grid, board->grid, sizeof(grid));

    if (board->moveCount == 0)
    {
        *row = BOARD_SIZE / 2;
        *col = BOARD_SIZE / 2;
        return AI_OK;
    }

    count = gen_candidates(grid, player, cand, 8);
    if (count == 0) return AI_ERR_NO_MOVE;

    for (i = 0; i < count; i++)
    {
        int wins = 0;
        for (sim = 0; sim < simsPerMove; sim++)
        {
            int g[BOARD_SIZE][BOARD_SIZE];
            int turn;
            int plies = 0;
            int result = 0;

            memcpy(g, grid, sizeof(g));
            g[cand[i].row][cand[i].col] = player;
            turn = OPPONENT(player);

            while (plies < 24)
            {
                int list[MAX_MOVES];
                int n;
                int pick;
                int r;
                int c;

                n = collect_empty_neighbors(g, list);
                if (n == 0) break;
                pick = rand() % n;
                r = list[pick] / BOARD_SIZE;
                c = list[pick] % BOARD_SIZE;
                g[r][c] = turn;
                plies++;
                if (has_five(g, r, c, turn))
                {
                    result = (turn == player) ? 1 : -1;
                    break;
                }
                turn = OPPONENT(turn);
            }

            if (result == 1) wins++;
        }

        if (wins > bestSim)
        {
            bestSim = wins;
            *row = cand[i].row;
            *col = cand[i].col;
        }
    }

    return AI_OK;
}
