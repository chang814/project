/*==============================================================================
 * 文件名称：ai_eval.c
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：AI 引擎模块 — 评估与候选生成子模块
 * 功能描述：实现棋型评分函数（pattern_score / point_score / has_five）、
 *           全盘局面评估（evaluate_side / live_threat_bonus / evaluate_grid）、
 *           以及候选点生成（gen_candidates 及其辅助函数）。
 * 创建日期：2026-08-26
 *============================================================================*/
#include "ai_internal.h"

#include <stdlib.h>
#include <string.h>

/*==============================================================================
 * 棋型评分
 *============================================================================*/

/* 根据连子长度和两端封堵数返回棋型分值 */
int pattern_score(int len, int blocked)
{
    if (len >= WIN_COUNT)
    {
        return AI_SCORE_FIVE;
    }
    if (blocked >= 2)
    {
        return 0;
    }

    switch (len)
    {
    case 4: return (blocked == 0) ? AI_SCORE_OPEN_FOUR   : AI_SCORE_FOUR;
    case 3: return (blocked == 0) ? AI_SCORE_OPEN_THREE  : AI_SCORE_SLEEP_THREE;
    case 2: return (blocked == 0) ? AI_SCORE_OPEN_TWO    : AI_SCORE_SLEEP_TWO;
    case 1: return (blocked == 0) ? AI_SCORE_ONE         : 0;
    default: return 0;
    }
}

/* 评估在 (row,col) 落 player 子后，该子在四个方向上的棋型总分 */
int point_score(const int grid[BOARD_SIZE][BOARD_SIZE],
                int row, int col, int player)
{
    int dirIndex;
    int total = 0;

    for (dirIndex = 0; dirIndex < DIR_COUNT; dirIndex++)
    {
        int deltaRow = AI_DIR[dirIndex][0];
        int deltaCol = AI_DIR[dirIndex][1];
        int len      = 1;
        int blocked  = 0;
        int r;
        int c;

        r = row + deltaRow;
        c = col + deltaCol;
        while (in_range(r, c) && grid[r][c] == player)
        {
            len++;
            r += deltaRow;
            c += deltaCol;
        }
        if (!in_range(r, c) || grid[r][c] != CELL_EMPTY) blocked++;

        r = row - deltaRow;
        c = col - deltaCol;
        while (in_range(r, c) && grid[r][c] == player)
        {
            len++;
            r -= deltaRow;
            c -= deltaCol;
        }
        if (!in_range(r, c) || grid[r][c] != CELL_EMPTY) blocked++;

        total += pattern_score(len, blocked);
    }

    return total;
}

/* 检查 (row,col) 的 player 子是否已形成五连 */
int has_five(const int grid[BOARD_SIZE][BOARD_SIZE],
             int row, int col, int player)
{
    int dirIndex;

    for (dirIndex = 0; dirIndex < DIR_COUNT; dirIndex++)
    {
        int deltaRow = AI_DIR[dirIndex][0];
        int deltaCol = AI_DIR[dirIndex][1];
        int len      = 1;
        int r;
        int c;

        r = row + deltaRow;
        c = col + deltaCol;
        while (in_range(r, c) && grid[r][c] == player)
        {
            len++;
            r += deltaRow;
            c += deltaCol;
        }
        r = row - deltaRow;
        c = col - deltaCol;
        while (in_range(r, c) && grid[r][c] == player)
        {
            len++;
            r -= deltaRow;
            c -= deltaCol;
        }
        if (len >= WIN_COUNT) return 1;
    }
    return 0;
}

/*==============================================================================
 * 全盘局面评估
 *============================================================================*/

/* 统计 player 方所有棋型的总分（按线段起点遍历，避免重复计数） */
int evaluate_side(const int grid[BOARD_SIZE][BOARD_SIZE], int player)
{
    int total = 0;
    int dirIndex;
    int row;
    int col;

    for (dirIndex = 0; dirIndex < DIR_COUNT; dirIndex++)
    {
        int deltaRow = AI_DIR[dirIndex][0];
        int deltaCol = AI_DIR[dirIndex][1];

        for (row = 0; row < BOARD_SIZE; row++)
        {
            for (col = 0; col < BOARD_SIZE; col++)
            {
                int prevRow;
                int prevCol;
                int endRow;
                int endCol;
                int len;
                int blocked;

                if (grid[row][col] != player) continue;

                prevRow = row - deltaRow;
                prevCol = col - deltaCol;
                if (in_range(prevRow, prevCol) && grid[prevRow][prevCol] == player)
                    continue;

                len = 0;
                endRow = row;
                endCol = col;
                while (in_range(endRow, endCol) && grid[endRow][endCol] == player)
                {
                    len++;
                    endRow += deltaRow;
                    endCol += deltaCol;
                }

                blocked = 0;
                if (!in_range(prevRow, prevCol) || grid[prevRow][prevCol] != CELL_EMPTY) blocked++;
                if (!in_range(endRow, endCol) || grid[endRow][endCol] != CELL_EMPTY) blocked++;

                total += pattern_score(len, blocked);
            }
        }
    }

    return total;
}

/* 威胁空间评估补充：统计独立活威胁数（≥活三），双活威胁权重放大 */
int live_threat_bonus(const int grid[BOARD_SIZE][BOARD_SIZE], int me)
{
    int threats = 0;
    int dirIndex;
    int row;
    int col;

    for (dirIndex = 0; dirIndex < DIR_COUNT; dirIndex++)
    {
        int dr = AI_DIR[dirIndex][0];
        int dc = AI_DIR[dirIndex][1];
        for (row = 0; row < BOARD_SIZE; row++)
        {
            for (col = 0; col < BOARD_SIZE; col++)
            {
                int prevRow = row - dr;
                int prevCol = col - dc;
                int len = 0;
                int endRow = row;
                int endCol = col;
                int blocked = 0;

                if (grid[row][col] != me) continue;
                if (in_range(prevRow, prevCol) && grid[prevRow][prevCol] == me) continue;

                while (in_range(endRow, endCol) && grid[endRow][endCol] == me)
                {
                    len++;
                    endRow += dr;
                    endCol += dc;
                }
                if (!in_range(prevRow, prevCol) || grid[prevRow][prevCol] != CELL_EMPTY) blocked++;
                if (!in_range(endRow, endCol) || grid[endRow][endCol] != CELL_EMPTY) blocked++;

                if (len >= WIN_COUNT)
                    threats += 3;
                else if (len == 4 && blocked <= 1)
                    threats += 2;
                else if (len == 3 && blocked == 0)
                    threats += 1;
            }
        }
    }

    /* 双威胁（双活三 / 四三）在五子棋中近乎必胜，分值应达到或超过活四；
     * 三威胁以上（双四 / 四三三）更是必杀。适度放大后，搜索既能主动
     * 布局“先造双威胁再收杀”的路线，也更容易识别并封堵对手的双威胁。 */
    if (threats >= 3) return AI_SCORE_OPEN_FOUR * 2;
    if (threats == 2) return AI_SCORE_OPEN_FOUR;
    if (threats == 1) return AI_SCORE_OPEN_THREE;
    return 0;
}

/* 评估整个局面对 me 的得分（含风格化攻防权重） */
int evaluate_grid(const int grid[BOARD_SIZE][BOARD_SIZE], int me)
{
    int mine = evaluate_side(grid, me);
    int hers = evaluate_side(grid, OPPONENT(me));

    mine += live_threat_bonus(grid, me);
    hers += live_threat_bonus(grid, OPPONENT(me));

    return (mine * s_attackW - hers * s_defendW) / 10;
}

/*==============================================================================
 * 候选生成辅助函数
 *============================================================================*/

/* 检查 (row,col) 周围 AI_NEIGHBOR_RANGE 范围内是否有棋子 */
int has_neighbor(const int grid[BOARD_SIZE][BOARD_SIZE], int row, int col)
{
    int dr;
    int dc;
    for (dr = -AI_NEIGHBOR_RANGE; dr <= AI_NEIGHBOR_RANGE; dr++)
    {
        for (dc = -AI_NEIGHBOR_RANGE; dc <= AI_NEIGHBOR_RANGE; dc++)
        {
            int r = row + dr;
            int c = col + dc;
            if (in_range(r, c) && grid[r][c] != CELL_EMPTY) return 1;
        }
    }
    return 0;
}

/* 收集所有有邻居的空位，以 row*BOARD_SIZE+col 编码返回 */
int collect_empty_neighbors(const int grid[BOARD_SIZE][BOARD_SIZE], int *out)
{
    int count = 0;
    int row;
    int col;
    for (row = 0; row < BOARD_SIZE; row++)
    {
        for (col = 0; col < BOARD_SIZE; col++)
        {
            if (grid[row][col] != CELL_EMPTY) continue;
            if (!has_neighbor(grid, row, col)) continue;
            out[count++] = row * BOARD_SIZE + col;
        }
    }
    return count;
}

/* 中心控制权：天元=2，周围3×3=1，更远=0 */
int center_bonus(int row, int col)
{
    int d = (abs(row - BOARD_SIZE / 2) > abs(col - BOARD_SIZE / 2))
            ? abs(row - BOARD_SIZE / 2) : abs(col - BOARD_SIZE / 2);
    return (d <= 1) ? (2 - d) : 0;
}

/* 势力范围：落子周围半径2内己方棋子数（0..12） */
int area_bonus(const int grid[BOARD_SIZE][BOARD_SIZE],
               int row, int col, int me)
{
    int cnt = 0;
    int r;
    int c;
    for (r = row - 2; r <= row + 2; r++)
    {
        for (c = col - 2; c <= col + 2; c++)
        {
            if (in_range(r, c) && grid[r][c] == me) cnt++;
        }
    }
    return cnt;
}

/* 技巧型：马步（日字）加分——落子周围8个骑士步位置中的己方棋子数 */
int knight_bonus(const int grid[BOARD_SIZE][BOARD_SIZE],
                 int row, int col, int me)
{
    static const int KNIGHT[8][2] =
    {
        { -1, -2 }, { -1, 2 }, { 1, -2 }, { 1, 2 },
        { -2, -1 }, { -2, 1 }, { 2, -1 }, { 2, 1 }
    };
    int cnt = 0;
    int i;
    for (i = 0; i < 8; i++)
    {
        int r = row + KNIGHT[i][0];
        int c = col + KNIGHT[i][1];
        if (in_range(r, c) && grid[r][c] == me) cnt++;
    }
    return cnt;
}

/* 落子后形成的活威胁方向数（≥活三）：双杀型双威胁检测 */
int threat_count_at(const int grid[BOARD_SIZE][BOARD_SIZE],
                    int row, int col, int me)
{
    int dirIndex;
    int t = 0;
    for (dirIndex = 0; dirIndex < DIR_COUNT; dirIndex++)
    {
        int dr = AI_DIR[dirIndex][0];
        int dc = AI_DIR[dirIndex][1];
        int len = 1;
        int blocked = 0;
        int r;
        int c;

        r = row + dr;
        c = col + dc;
        while (in_range(r, c) && grid[r][c] == me) { len++; r += dr; c += dc; }
        if (!in_range(r, c) || grid[r][c] != CELL_EMPTY) blocked++;

        r = row - dr;
        c = col - dc;
        while (in_range(r, c) && grid[r][c] == me) { len++; r -= dr; c -= dc; }
        if (!in_range(r, c) || grid[r][c] != CELL_EMPTY) blocked++;

        if (len >= 3 && pattern_score(len, blocked) >= AI_SCORE_OPEN_THREE) t++;
    }
    return t;
}

/*==============================================================================
 * 候选点生成主函数
 *============================================================================*/

/* 生成 turn 方的候选落子点，按评分降序排列，最多返回 maxOut 个 */
int gen_candidates(const int grid[BOARD_SIZE][BOARD_SIZE], int turn,
                   AiCandidate *out, int maxOut)
{
    AiCandidate pool[MAX_MOVES];
    int cellList[MAX_MOVES];
    int poolCount = 0;
    int opponent  = OPPONENT(turn);
    int listCount;
    int i;
    int j;
    int keep;

    listCount = collect_empty_neighbors(grid, cellList);

    for (i = 0; i < listCount; i++)
    {
        int row = cellList[i] / BOARD_SIZE;
        int col = cellList[i] % BOARD_SIZE;
        int score = point_score(grid, row, col, turn) * s_candAtkMul +
                    point_score(grid, row, col, opponent) * s_candDefMul;

        /* 控场型：中心控制权 + 势力范围延伸加分 */
        if (s_candCenterW != 0) score += center_bonus(row, col) * s_candCenterW;
        if (s_candAreaW != 0)   score += area_bonus(grid, row, col, turn) * s_candAreaW;
        /* 技巧型：马步（日字）布局加分 */
        if (s_candKnightW != 0)
        {
            int kb = knight_bonus(grid, row, col, turn);
            if (kb > 0) score += kb * s_candKnightW;
        }
        /* 技巧型：阵法候选加分——落此子使阵法成型则加分 */
        if (s_style == STYLE_TRICK && s_candFormW != 0)
        {
            int tmp[BOARD_SIZE][BOARD_SIZE];
            memcpy(tmp, grid, sizeof(tmp));
            tmp[row][col] = turn;
            if (formation_from_grid(tmp, turn) != FORMATION_NONE)
                score += s_candFormW;
        }
        /* 双杀型：一步形成双威胁（≥活三）的点额外 ×3 */
        if (s_style == STYLE_DOUBLE && threat_count_at(grid, row, col, turn) >= 2)
            score *= 3;

        /* 防守保障：若对手在此落子即成五/活四，此点必须进入候选 */
        if (point_score(grid, row, col, opponent) >= AI_SCORE_FIVE)
        {
            if (score < AI_SCORE_OPEN_FOUR) score = AI_SCORE_OPEN_FOUR;
        }
        else if (point_score(grid, row, col, opponent) >= AI_SCORE_FOUR)
        {
            if (score < AI_SCORE_FOUR - 1) score = AI_SCORE_FOUR - 1;
        }

        pool[poolCount].row   = row;
        pool[poolCount].col   = col;
        pool[poolCount].score = score;
        poolCount++;
    }

    keep = (poolCount < maxOut) ? poolCount : maxOut;

    /* 选择排序取前 keep 个最高分 */
    for (i = 0; i < keep; i++)
    {
        int best = i;
        for (j = i + 1; j < poolCount; j++)
        {
            if (pool[j].score > pool[best].score) best = j;
        }
        if (best != i)
        {
            AiCandidate tmp = pool[i];
            pool[i] = pool[best];
            pool[best] = tmp;
        }
        out[i] = pool[i];
    }

    return keep;
}
