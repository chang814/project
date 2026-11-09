/*==============================================================================
 * 文件名称：ai_search.c
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：AI 引擎模块 — 搜索算法子模块
 * 功能描述：实现六种核心搜索算法（随机/贪心/Minimax/Alpha-Beta/IDDFS/PVS）、
 *           置换表（Zobrist Hash）操作、以及移动排序启发（杀手/历史启发）。
 * 创建日期：2026-08-26
 *============================================================================*/
#include "ai_internal.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

/*==============================================================================
 * 移动排序启发（大师档及以上解锁）
 * 历史启发：全局记录导致剪枝的走法，按其加分重排候选。
 * 杀手启发：每层保存 2 个导致剪枝的走法，搜索时提到该层候选最前。
 *============================================================================*/

void killer_store(int depth, int row, int col)
{
    int mv = row * BOARD_SIZE + col;
    if (s_killer[depth][0] != mv)
    {
        s_killer[depth][1] = s_killer[depth][0];
        s_killer[depth][0] = mv;
    }
}

void sort_by_killer(AiCandidate *cand, int count, int depth)
{
    int k;
    for (k = 0; k < 2; k++)
    {
        int mv = s_killer[depth][k];
        int i;
        int j;
        if (mv <= 0) continue;
        for (i = 0; i < count; i++)
        {
            if (cand[i].row * BOARD_SIZE + cand[i].col == mv && i > k)
            {
                AiCandidate tmp = cand[i];
                for (j = i; j > k; j--) cand[j] = cand[j - 1];
                cand[k] = tmp;
                break;
            }
        }
    }
}

void sort_by_history(AiCandidate *cand, int count)
{
    int i;
    int j;
    for (i = 0; i < count; i++)
    {
        int best = i;
        for (j = i + 1; j < count; j++)
        {
            if (cand[j].score + s_history[cand[j].row][cand[j].col] >
                cand[best].score + s_history[cand[best].row][cand[best].col])
                best = j;
        }
        if (best != i)
        {
            AiCandidate tmp = cand[i];
            cand[i] = cand[best];
            cand[best] = tmp;
        }
    }
}

/*==============================================================================
 * 置换表（Zobrist Hash）
 * 以 64 位哈希缓存已搜索局面，避免相同局面重复计算（困难档及以上解锁）。
 * 条目带 depth/flag(EXACT/LOWER/UPPER)，仅在深度足够时复用。
 * 跨手数复用：同一对局中不清空置换表，Zobrist 哈希编码完整局面，
 * 不同手数的局面哈希不同，不会冲突。新对局开始时由 ai_init 清空一次。
 *============================================================================*/

void tt_clear(void)
{
    memset(s_tt, 0, sizeof(s_tt));
}

TtEntry *tt_probe(U64 hash)
{
    TtEntry *e = &s_tt[hash & TT_MASK];
    if (s_ttOn && e->hash == hash && e->depth > 0) return e;
    return NULL;
}

void tt_store(U64 hash, int depth, int flag, int score,
              int bestRow, int bestCol)
{
    TtEntry *e;
    if (!s_ttOn) return;
    e = &s_tt[hash & TT_MASK];
    e->hash    = hash;
    e->depth   = depth;
    e->flag    = flag;
    e->score   = score;
    e->bestRow = bestRow;
    e->bestCol = bestCol;
}

/* 整盘计算 Zobrist 哈希（每次决策仅一次，搜索中增量更新） */
U64 board_hash(const int grid[BOARD_SIZE][BOARD_SIZE])
{
    U64 h = 0;
    int row;
    int col;
    for (row = 0; row < BOARD_SIZE; row++)
    {
        for (col = 0; col < BOARD_SIZE; col++)
        {
            int s = grid[row][col];
            if (s != CELL_EMPTY) h ^= s_zobrist[s][row][col];
        }
    }
    return h;
}

/*==============================================================================
 * 搜索公共辅助（去重：alphabeta 与 pvs 共享的逻辑）
 *============================================================================*/

/* 生成候选并应用移动排序启发 */
static int search_gen_ordered(int grid[BOARD_SIZE][BOARD_SIZE], int turn,
                               int width, int depth, AiCandidate *cand)
{
    int count = gen_candidates(grid, turn, cand, width);
    /* 杀手启发必须用当前结点深度：原实现写死 depth=0，导致每层的杀手
     * 走法永远不会命中本层排序，启发基本失效，拖慢剪枝、搜不深 */
    if (s_useKiller)  sort_by_killer(cand, count, depth);
    if (s_useHistory) sort_by_history(cand, count);
    return count;
}

/* 记录剪枝走法（历史启发 + 杀手启发） */
static void search_record_cutoff(int depth, int row, int col)
{
    if (s_useHistory) s_history[row][col] += depth * depth;
    if (s_useKiller)  killer_store(depth, row, col);
}

/* 置换表存储：超时不存，必胜/必败终局值不存（避免深度错配） */
static void search_tt_store(U64 hash, int depth, int alpha, int beta,
                             int best, int bestRow, int bestCol)
{
    if (s_timeout) return;
    if (best >= AI_WIN_SCORE / 2 || best <= -AI_WIN_SCORE / 2) return;

    int flag = TT_EXACT;
    if (best <= alpha) flag = TT_UPPER;
    else if (best >= beta) flag = TT_LOWER;
    tt_store(hash, depth, flag, best,
             (bestRow >= 0) ? bestRow : -1,
             (bestCol >= 0) ? bestCol : -1);
}

/*==============================================================================
 * Alpha-Beta 剪枝搜索（困难档）
 * 在极小极大基础上引入 alpha(下界)/beta(上界)：当某个分支的结果
 * 不可能影响最终决策时直接剪掉，大幅减少搜索节点数。
 * 配合时间控制(s_deadline)可搜索更深；大师/棋圣档的迭代加深循环内部
 * 同样调用本函数。困难档及以上启用置换表：查表命中直接剪枝，
 * 落子/回退时对哈希做增量 XOR，避免整盘重算。
 *============================================================================*/
int alphabeta_search(int grid[BOARD_SIZE][BOARD_SIZE], int depth,
                     int alpha, int beta, int me, int turn, int width,
                     U64 hash, int *bestRow, int *bestCol)
{
    AiCandidate candidate[AI_MAX_CANDIDATE];
    TtEntry *e;
    int count;
    int i;
    int best;

    if (bestRow != NULL) { *bestRow = -1; *bestCol = -1; }

    if (clock() > s_deadline)
    {
        s_timeout = 1;
        return evaluate_grid(grid, me);
    }
    if (depth <= 0) return evaluate_grid(grid, me);

    /* 置换表查表：深度足够时按 flag 直接剪枝 */
    e = tt_probe(hash);
    if (e != NULL && e->depth >= depth)
    {
        if (e->flag == TT_EXACT)
        {
            if (bestRow != NULL) { *bestRow = e->bestRow; *bestCol = e->bestCol; }
            return e->score;
        }
        if (e->flag == TT_LOWER && e->score >= beta) return e->score;
        if (e->flag == TT_UPPER && e->score <= alpha) return e->score;
    }

    count = search_gen_ordered(grid, turn, width, depth, candidate);
    if (count == 0) return evaluate_grid(grid, me);

    if (turn == me)
    {
        best = -AI_INFINITY;
        for (i = 0; i < count; i++)
        {
            int row = candidate[i].row;
            int col = candidate[i].col;
            int value;

            grid[row][col] = turn;
            if (has_five(grid, row, col, turn))
                value = AI_WIN_SCORE + depth;
            else
                value = alphabeta_search(grid, depth - 1, alpha, beta, me, OPPONENT(turn),
                                         width, hash ^ s_zobrist[turn][row][col], NULL, NULL);
            grid[row][col] = CELL_EMPTY;

            if (value > best)
            {
                best = value;
                if (bestRow != NULL) { *bestRow = row; *bestCol = col; }
            }
            if (best > alpha) alpha = best;
            if (alpha >= beta)
            {
                search_record_cutoff(depth, row, col);
                break;
            }
            if (s_timeout) break;
        }
    }
    else
    {
        best = AI_INFINITY;
        for (i = 0; i < count; i++)
        {
            int row = candidate[i].row;
            int col = candidate[i].col;
            int value;

            grid[row][col] = turn;
            if (has_five(grid, row, col, turn))
                value = -AI_WIN_SCORE - depth;
            else
                value = alphabeta_search(grid, depth - 1, alpha, beta, me, OPPONENT(turn),
                                         width, hash ^ s_zobrist[turn][row][col], NULL, NULL);
            grid[row][col] = CELL_EMPTY;

            if (value < best)
            {
                best = value;
                if (bestRow != NULL) { *bestRow = row; *bestCol = col; }
            }
            if (best < beta) beta = best;
            if (alpha >= beta)
            {
                search_record_cutoff(depth, row, col);
                break;
            }
            if (s_timeout) break;
        }
    }

    search_tt_store(hash, depth, alpha, beta, best,
                     (bestRow != NULL) ? *bestRow : -1,
                     (bestCol != NULL) ? *bestCol : -1);
    return best;
}

/*==============================================================================
 * PVS 主要变例搜索（棋圣档）
 * Alpha-Beta 的进阶：假设移动排序后的第一个候选是最优的（主要变例），
 * 后续候选先用零宽度窗口(alpha, alpha+1)快速验证；验证失败（可能更优）
 * 再用完整窗口重搜。"先假设后验证"比普通 Alpha-Beta 剪掉更多分支。
 * 棋圣档同时启用全部优化：置换表 + 历史/杀手启发 + 空着裁剪（可选）。
 *============================================================================*/
int pvs_search(int grid[BOARD_SIZE][BOARD_SIZE], int depth,
               int alpha, int beta, int me, int turn, int width,
               U64 hash, int *bestRow, int *bestCol)
{
    AiCandidate candidate[AI_MAX_CANDIDATE];
    TtEntry *e;
    int count;
    int i;
    int best;

    if (bestRow != NULL) { *bestRow = -1; *bestCol = -1; }

    if (clock() > s_deadline)
    {
        s_timeout = 1;
        return evaluate_grid(grid, me);
    }
    if (depth <= 0) return evaluate_grid(grid, me);

    /* 空着裁剪（可选演示开关，默认关闭）：我方回合跳过一手，
     * 若对手连走两步我方仍占优则剪枝。五子棋无让先，会漏对手
     * 连续威胁，故仅作答辩演示用途。 */
    if (s_nullMoveOn && turn == me && depth >= 3 && bestRow == NULL)
    {
        int nullVal = pvs_search(grid, depth - 2, -beta, -beta + 1, me,
                                 OPPONENT(turn), width, hash, NULL, NULL);
        if (nullVal >= beta) return beta;
    }

    /* 置换表查表 */
    e = tt_probe(hash);
    if (e != NULL && e->depth >= depth)
    {
        if (e->flag == TT_EXACT)
        {
            if (bestRow != NULL) { *bestRow = e->bestRow; *bestCol = e->bestCol; }
            return e->score;
        }
        if (e->flag == TT_LOWER && e->score >= beta) return e->score;
        if (e->flag == TT_UPPER && e->score <= alpha) return e->score;
    }

    count = search_gen_ordered(grid, turn, width, depth, candidate);
    if (count == 0) return evaluate_grid(grid, me);

    if (turn == me)
    {
        best = -AI_INFINITY;
        for (i = 0; i < count; i++)
        {
            int row = candidate[i].row;
            int col = candidate[i].col;
            int value;

            grid[row][col] = turn;
            if (has_five(grid, row, col, turn))
                value = AI_WIN_SCORE + depth;
            else if (i == 0)
                value = pvs_search(grid, depth - 1, alpha, beta, me, OPPONENT(turn),
                                   width, hash ^ s_zobrist[turn][row][col], NULL, NULL);
            else
            {
                value = pvs_search(grid, depth - 1, alpha, alpha + 1, me, OPPONENT(turn),
                                   width, hash ^ s_zobrist[turn][row][col], NULL, NULL);
                if (value > alpha && value < beta)   /* 零窗口验证失败：可能更优，重搜 */
                    value = pvs_search(grid, depth - 1, alpha, beta, me, OPPONENT(turn),
                                       width, hash ^ s_zobrist[turn][row][col], NULL, NULL);
            }
            grid[row][col] = CELL_EMPTY;

            if (value > best)
            {
                best = value;
                if (bestRow != NULL) { *bestRow = row; *bestCol = col; }
            }
            if (best > alpha) alpha = best;
            if (alpha >= beta)
            {
                search_record_cutoff(depth, row, col);
                break;
            }
            if (s_timeout) break;
        }
    }
    else
    {
        best = AI_INFINITY;
        for (i = 0; i < count; i++)
        {
            int row = candidate[i].row;
            int col = candidate[i].col;
            int value;

            grid[row][col] = turn;
            if (has_five(grid, row, col, turn))
                value = -AI_WIN_SCORE - depth;
            else if (i == 0)
                value = pvs_search(grid, depth - 1, alpha, beta, me, OPPONENT(turn),
                                   width, hash ^ s_zobrist[turn][row][col], NULL, NULL);
            else
            {
                value = pvs_search(grid, depth - 1, beta - 1, beta, me, OPPONENT(turn),
                                   width, hash ^ s_zobrist[turn][row][col], NULL, NULL);
                if (value < beta && value > alpha)
                    value = pvs_search(grid, depth - 1, alpha, beta, me, OPPONENT(turn),
                                       width, hash ^ s_zobrist[turn][row][col], NULL, NULL);
            }
            grid[row][col] = CELL_EMPTY;

            if (value < best)
            {
                best = value;
                if (bestRow != NULL) { *bestRow = row; *bestCol = col; }
            }
            if (best < beta) beta = best;
            if (alpha >= beta)
            {
                search_record_cutoff(depth, row, col);
                break;
            }
            if (s_timeout) break;
        }
    }

    search_tt_store(hash, depth, alpha, beta, best,
                     (bestRow != NULL) ? *bestRow : -1,
                     (bestCol != NULL) ? *bestCol : -1);
    return best;
}

/*==============================================================================
 * 纯极小极大搜索（普通档）
 * 最原始的对抗搜索：无任何剪枝，完整遍历博弈树。
 * 己方节点取最大值(max)，对方节点取最小值(min)，深度 2 层即可体现
 * 对抗思想。与 alphabeta_search 的差异正是"无剪枝"，用于展示博弈
 * 搜索的演进过程（Minimax -> Alpha-Beta -> PVS）。
 *============================================================================*/
int minimax_search(int grid[BOARD_SIZE][BOARD_SIZE], int depth,
                   int me, int turn, int width, int *bestRow, int *bestCol)
{
    AiCandidate candidate[AI_MAX_CANDIDATE];
    int count;
    int i;
    int best;

    if (bestRow != NULL) { *bestRow = -1; *bestCol = -1; }
    if (depth <= 0) return evaluate_grid(grid, me);

    count = gen_candidates(grid, turn, candidate, width);
    if (count == 0) return evaluate_grid(grid, me);

    if (turn == me)
    {
        best = -AI_INFINITY;
        for (i = 0; i < count; i++)
        {
            int row = candidate[i].row;
            int col = candidate[i].col;
            int value;

            grid[row][col] = turn;
            if (has_five(grid, row, col, turn))
                value = AI_WIN_SCORE + depth;
            else
                value = minimax_search(grid, depth - 1, me, OPPONENT(turn), width, NULL, NULL);
            grid[row][col] = CELL_EMPTY;

            if (value > best)
            {
                best = value;
                if (bestRow != NULL) { *bestRow = row; *bestCol = col; }
            }
        }
    }
    else
    {
        best = AI_INFINITY;
        for (i = 0; i < count; i++)
        {
            int row = candidate[i].row;
            int col = candidate[i].col;
            int value;

            grid[row][col] = turn;
            if (has_five(grid, row, col, turn))
                value = -AI_WIN_SCORE - depth;
            else
                value = minimax_search(grid, depth - 1, me, OPPONENT(turn), width, NULL, NULL);
            grid[row][col] = CELL_EMPTY;

            if (value < best)
            {
                best = value;
                if (bestRow != NULL) { *bestRow = row; *bestCol = col; }
            }
        }
    }

    return best;
}

/*==============================================================================
 * 随机算法（入门档）
 * 在所有有邻居的空位中完全随机落子，无任何策略，作为棋力基线。
 *============================================================================*/
int random_choose(const int grid[BOARD_SIZE][BOARD_SIZE], int *bestRow, int *bestCol)
{
    int cellList[MAX_MOVES];
    int count;
    int pick;

    count = collect_empty_neighbors(grid, cellList);
    if (count == 0) return AI_ERR_NO_MOVE;

    pick = rand() % count;
    *bestRow = cellList[pick] / BOARD_SIZE;
    *bestCol = cellList[pick] % BOARD_SIZE;
    return AI_OK;
}

/*==============================================================================
 * 贪心算法（简单档）
 * 一步启发式评估：取即时局面得分最高的点，不做多步预判。
 * 并列最高分随机打破，避免同局面永远走同一手。
 *============================================================================*/
int greedy_choose(const int grid[BOARD_SIZE][BOARD_SIZE], int me, int width,
                  int *bestRow, int *bestCol)
{
    AiCandidate candidate[AI_MAX_CANDIDATE];
    int tie[MAX_MOVES];
    int count;
    int best;
    int tieCount;
    int i;

    count = gen_candidates(grid, me, candidate, width);
    if (count == 0) return AI_ERR_NO_MOVE;

    best = 0;
    for (i = 1; i < count; i++)
    {
        if (candidate[i].score > candidate[best].score) best = i;
    }

    tieCount = 0;
    for (i = 0; i < count; i++)
    {
        if (candidate[i].score == candidate[best].score) tie[tieCount++] = i;
    }

    best = tie[rand() % tieCount];
    *bestRow = candidate[best].row;
    *bestCol = candidate[best].col;
    return AI_OK;
}

/*==============================================================================
 * 迭代加深 IDDFS（大师档）
 * 从深度 2 开始逐层加深，每层结果作为下一层搜索的起点（移动排序参考），
 * 达到时间预算立即停止并返回当前最深层结果。
 *============================================================================*/
int iddfs_loop(int grid[BOARD_SIZE][BOARD_SIZE], int me, int maxDepth, int width,
                U64 hash, int *bestRow, int *bestCol)
{
    AiCandidate candidate[AI_MAX_CANDIDATE];
    int count;
    int i;

    count = gen_candidates(grid, me, candidate, width);
    if (count == 0) return AI_ERR_NO_MOVE;

    *bestRow = candidate[0].row;
    *bestCol = candidate[0].col;

    for (i = 2; i <= maxDepth; i += 2)
    {
        int depthRow = -1;
        int depthCol = -1;

        alphabeta_search(grid, i, -AI_INFINITY, AI_INFINITY, me, me,
                         width, hash, &depthRow, &depthCol);

        if (s_timeout) break;
        if (depthRow >= 0)
        {
            *bestRow    = depthRow;
            *bestCol    = depthCol;
            s_lastDepth = i;
        }
        if (clock() > s_deadline - (clock_t)(s_budgetMs / 2 * CLOCKS_PER_SEC / 1000))
            break;
    }

    return AI_OK;
}

/* 迭代加深 PVS 版（棋圣档）：逐层加深，每层用 PVS 零窗口搜索 */
int iddfs_pvs_loop(int grid[BOARD_SIZE][BOARD_SIZE], int me, int maxDepth, int width,
                    U64 hash, int *bestRow, int *bestCol)
{
    AiCandidate candidate[AI_MAX_CANDIDATE];
    int count;
    int i;
    int seedIdx;

    count = gen_candidates(grid, me, candidate, width);
    if (count == 0) return AI_ERR_NO_MOVE;

    /* 技巧型：5% 概率选次优种子，增加不可预测性 */
    seedIdx = (s_style == STYLE_TRICK && count > 1 && (rand() % 100) < 5) ? 1 : 0;
    *bestRow = candidate[seedIdx].row;
    *bestCol = candidate[seedIdx].col;

    for (i = 2; i <= maxDepth; i += 2)
    {
        int depthRow = -1;
        int depthCol = -1;

        pvs_search(grid, i, -AI_INFINITY, AI_INFINITY, me, me,
                   width, hash, &depthRow, &depthCol);

        if (s_timeout) break;
        if (depthRow >= 0)
        {
            *bestRow    = depthRow;
            *bestCol    = depthCol;
            s_lastDepth = i;
        }
        if (clock() > s_deadline - (clock_t)(s_budgetMs / 2 * CLOCKS_PER_SEC / 1000))
            break;
    }

    return AI_OK;
}

/* 根据难度档位配置最大搜索深度和候选宽度 */
void level_config(int level, int *maxDepth, int *width)
{
    switch (level)
    {
    case LEVEL_BEGINNER:
        *maxDepth = 0;              /* 随机算法：不搜索 */
        *width    = 8;
        break;
    case LEVEL_EASY:
        *maxDepth = 0;              /* 贪心：一步启发，取评分前 8 */
        *width    = 8;
        break;
    case LEVEL_NORMAL:
        *maxDepth = 4;              /* 纯极小极大（偶数层，评估落在对手刚走完，无奇偶偏差） */
        *width    = 8;
        break;
    case LEVEL_HARD:
        *maxDepth = 6;              /* Alpha-Beta 剪枝 */
        *width    = 10;
        break;
    case LEVEL_MASTER:
        *maxDepth = 8;              /* 迭代加深 IDDFS（预算内加深） */
        *width    = 10;
        break;
    case LEVEL_GRANDMASTER:
        *maxDepth = 12;             /* 棋圣深算：VCF 杀路 + 迭代加深 + 专属长时预算 */
        *width    = 10;
        break;
    default:
        *maxDepth = 2;
        *width    = 8;
        break;
    }
}
