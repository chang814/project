/*==============================================================================
 * 文件名称：ai_vcf.c
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：AI 引擎模块 — VCF/VCT 杀棋检测子模块
 * 功能描述：实现连续冲四取胜（VCF）和连续活三取胜（VCT）的递归检测。
 *           VCF：通过"冲四→对手被迫堵→再冲四"的强制序列连续进攻直至五连。
 *           VCT：在 VCF 基础上放宽起点，活三/冲四/活四创造点均可发起。
 * 创建日期：2026-08-26
 *============================================================================*/
#include "ai_internal.h"

#include <string.h>

/* ---- VCF 参数 ---- */
#define VCF_MAX_DEPTH    6
#define VCF_NODE_LIMIT   20000
static int s_vcfNodes = 0;

/* ---- VCT 参数 ---- */
#define VCT_MAX_DEPTH    5
#define VCT_NODE_LIMIT   20000
static int s_vctNodes = 0;

/*==============================================================================
 * VCF：连续冲四取胜
 *============================================================================*/

/* 收集所有"落子即成五"的空点 */
int collect_five_points(const int grid[BOARD_SIZE][BOARD_SIZE], int me,
                        int *rows, int *cols, int cap)
{
    int list[MAX_MOVES];
    int count;
    int i;
    int n = 0;

    count = collect_empty_neighbors(grid, list);
    for (i = 0; i < count && n < cap; i++)
    {
        int row = list[i] / BOARD_SIZE;
        int col = list[i] % BOARD_SIZE;
        if (point_score(grid, row, col, me) >= AI_SCORE_FIVE)
        {
            rows[n] = row;
            cols[n] = col;
            n++;
        }
    }
    return n;
}

/* 收集"落子后形成 ≥冲四"的冲四/活四创造点，按评分降序排列 */
int gen_si_points(const int grid[BOARD_SIZE][BOARD_SIZE], int me,
                   AiCandidate *out, int maxOut)
{
    int list[MAX_MOVES];
    int count;
    int i;
    int n = 0;

    count = collect_empty_neighbors(grid, list);
    for (i = 0; i < count && n < maxOut; i++)
    {
        int row = list[i] / BOARD_SIZE;
        int col = list[i] % BOARD_SIZE;
        int s   = point_score(grid, row, col, me);
        if (s >= AI_SCORE_FOUR && s < AI_SCORE_FIVE)
        {
            out[n].row   = row;
            out[n].col   = col;
            out[n].score = s;
            n++;
        }
    }

    /* 多条杀路时优先最强的（双威胁 > 单活四 > 冲四） */
    for (i = 0; i < n; i++)
    {
        int best = i;
        int j;
        for (j = i + 1; j < n; j++)
        {
            if (out[j].score > out[best].score) best = j;
        }
        if (best != i)
        {
            AiCandidate tmp = out[i];
            out[i] = out[best];
            out[best] = tmp;
        }
    }
    return n;
}

/* VCF 递归：depth 层内连续冲四必胜。成功返回 1，outRow/outCol 为第一步 */
int vcf_search(int grid[BOARD_SIZE][BOARD_SIZE], int me, int depth,
               int *outRow, int *outCol)
{
    AiCandidate si[AI_MAX_CANDIDATE];
    int r[16];
    int c[16];
    int n;
    int i;
    int j;

    s_vcfNodes++;
    if (s_vcfNodes > VCF_NODE_LIMIT) return 0;
    if (depth <= 0) return 0;

    /* 对手有一步成五：我方必须先防守，VCF 不成立 */
    if (collect_five_points(grid, OPPONENT(me), r, c, 1) > 0) return 0;

    /* 我方一步成五：直接赢 */
    n = collect_five_points(grid, me, r, c, 16);
    if (n > 0)
    {
        *outRow = r[0];
        *outCol = c[0];
        return 1;
    }

    /* 尝试每个冲四/活四创造点：对手必须堵，堵不完即我方赢 */
    n = gen_si_points(grid, me, si, AI_MAX_CANDIDATE);
    for (i = 0; i < n; i++)
    {
        int row = si[i].row;
        int col = si[i].col;
        int all  = 1;

        grid[row][col] = me;

        {
            int br[16];
            int bc[16];
            int bn = collect_five_points(grid, me, br, bc, 16);

            if (bn == 0)
            {
                all = 0;   /* 落子未形成成五点，非有效冲四 */
            }
            else
            {
                /* 对手必然选择对其最有利的堵点，全部堵点我方都能杀才算必胜 */
                for (j = 0; j < bn && all; j++)
                {
                    grid[br[j]][bc[j]] = OPPONENT(me);
                    if (has_five(grid, br[j], bc[j], OPPONENT(me)))
                        all = 0;   /* 对手堵点同时成五：对手赢 */
                    else if (!vcf_search(grid, me, depth - 1, outRow, outCol))
                        all = 0;   /* 该堵法下我方无法继续连杀 */
                    grid[br[j]][bc[j]] = CELL_EMPTY;
                }
            }
        }

        grid[row][col] = CELL_EMPTY;
        if (all)
        {
            *outRow = row;
            *outCol = col;
            return 1;
        }
    }
    return 0;
}

/* VCF 检测入口：在副本棋盘上执行，不影响后续搜索 */
int detect_vcf(const int grid[BOARD_SIZE][BOARD_SIZE], int me,
               int *outRow, int *outCol)
{
    int g[BOARD_SIZE][BOARD_SIZE];

    s_vcfNodes = 0;
    memcpy(g, grid, sizeof(g));
    return vcf_search(g, me, VCF_MAX_DEPTH, outRow, outCol);
}

/*==============================================================================
 * VCT：连续活三取胜（在 VCF 基础上放宽起点）
 *============================================================================*/

/* 收集"活威胁创造点"：落子后 ≥活三（含冲四、活四），排除一步成五 */
int gen_threat_points(const int grid[BOARD_SIZE][BOARD_SIZE], int me,
                      AiCandidate *out, int maxOut)
{
    int list[MAX_MOVES];
    int count;
    int i;
    int n = 0;

    count = collect_empty_neighbors(grid, list);
    for (i = 0; i < count && n < maxOut; i++)
    {
        int row = list[i] / BOARD_SIZE;
        int col = list[i] % BOARD_SIZE;
        int s   = point_score(grid, row, col, me);
        if (s >= AI_SCORE_OPEN_THREE && s < AI_SCORE_FIVE)
        {
            out[n].row   = row;
            out[n].col   = col;
            out[n].score = s;
            n++;
        }
    }

    /* 多条杀路时优先最强的 */
    for (i = 0; i < n; i++)
    {
        int best = i;
        int j;
        for (j = i + 1; j < n; j++)
        {
            if (out[j].score > out[best].score) best = j;
        }
        if (best != i)
        {
            AiCandidate tmp = out[i];
            out[i] = out[best];
            out[best] = tmp;
        }
    }
    return n;
}

/* 落子 (row,col) 后若形成活三（某方向 len=3 两端空），返回两端延伸点 */
int open_three_ends(const int grid[BOARD_SIZE][BOARD_SIZE],
                    int row, int col, int me,
                    int *endsR, int *endsC, int cap)
{
    int dirIndex;
    int n = 0;

    for (dirIndex = 0; dirIndex < DIR_COUNT; dirIndex++)
    {
        int dr = AI_DIR[dirIndex][0];
        int dc = AI_DIR[dirIndex][1];
        int left  = 0;
        int right = 0;
        int r;
        int c;

        r = row - dr;
        c = col - dc;
        while (in_range(r, c) && grid[r][c] == me) { left++; r -= dr; c -= dc; }
        r = row + dr;
        c = col + dc;
        while (in_range(r, c) && grid[r][c] == me) { right++; r += dr; c += dc; }

        if (left + right + 1 != 3) continue;          /* 非活三长度 */
        if (!in_range(r, c) || grid[r][c] != CELL_EMPTY) continue;  /* 右端堵 */
        if (!in_range(row - dr * (left + 1), col - dc * (left + 1))) continue;
        if (grid[row - dr * (left + 1)][col - dc * (left + 1)] != CELL_EMPTY) continue;

        if (n < cap) { endsR[n] = r; endsC[n] = c; n++; }
        if (n < cap)
        {
            endsR[n] = row - dr * (left + 1);
            endsC[n] = col - dc * (left + 1);
            n++;
        }
    }
    return n;
}

/* VCT 递归：活三/冲四连续威胁直至五连。成功返回 1，outRow/outCol 为第一步 */
int vct_search(int grid[BOARD_SIZE][BOARD_SIZE], int me, int depth,
               int *outRow, int *outCol)
{
    AiCandidate tp[AI_MAX_CANDIDATE];
    int r[16];
    int c[16];
    int n;
    int i;
    int j;

    s_vctNodes++;
    if (s_vctNodes > VCT_NODE_LIMIT) return 0;
    if (depth <= 0) return 0;

    /* 对手有一步成五：我方必须先防守 */
    if (collect_five_points(grid, OPPONENT(me), r, c, 1) > 0) return 0;

    /* 我方一步成五：直接赢 */
    n = collect_five_points(grid, me, r, c, 16);
    if (n > 0)
    {
        *outRow = r[0];
        *outCol = c[0];
        return 1;
    }

    n = gen_threat_points(grid, me, tp, AI_MAX_CANDIDATE);
    for (i = 0; i < n; i++)
    {
        int row = tp[i].row;
        int col = tp[i].col;
        int all  = 1;

        grid[row][col] = me;

        {
            int br[16];
            int bc[16];
            int bn = collect_five_points(grid, me, br, bc, 16);

            if (bn == 0)
            {
                /* 只形成活三：对手堵活三延伸端之一 */
                int er[2];
                int ec[2];
                int en = open_three_ends(grid, row, col, me, er, ec, 2);
                if (en == 0) all = 0;
                for (j = 0; j < en && all; j++)
                {
                    grid[er[j]][ec[j]] = OPPONENT(me);
                    if (has_five(grid, er[j], ec[j], OPPONENT(me)))
                        all = 0;
                    else if (!vct_search(grid, me, depth - 1, outRow, outCol))
                        all = 0;
                    grid[er[j]][ec[j]] = CELL_EMPTY;
                }
            }
            else
            {
                /* 冲四/活四：对手必须堵成五点，全部堵法我方都胜才算必胜 */
                for (j = 0; j < bn && all; j++)
                {
                    grid[br[j]][bc[j]] = OPPONENT(me);
                    if (has_five(grid, br[j], bc[j], OPPONENT(me)))
                        all = 0;
                    else if (!vct_search(grid, me, depth - 1, outRow, outCol))
                        all = 0;
                    grid[br[j]][bc[j]] = CELL_EMPTY;
                }
            }
        }

        grid[row][col] = CELL_EMPTY;
        if (all)
        {
            *outRow = row;
            *outCol = col;
            return 1;
        }
    }
    return 0;
}

/* VCT 检测入口：在副本棋盘上执行，不影响后续搜索 */
int detect_vct(const int grid[BOARD_SIZE][BOARD_SIZE], int me,
               int *outRow, int *outCol)
{
    int g[BOARD_SIZE][BOARD_SIZE];

    s_vctNodes = 0;
    memcpy(g, grid, sizeof(g));
    return vct_search(g, me, VCT_MAX_DEPTH, outRow, outCol);
}
