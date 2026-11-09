/*==============================================================================
 * 文件名称：ai_formation.c
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：AI 引擎模块 — 阵型识别与开局库子模块
 * 功能描述：实现 7 种民间基础阵法的检测（梅花/八卦/燕/流星/斜三/四角/十字）、
 *           阵型解说文本生成、技巧型候选点阵法加分，以及棋圣档强开局库。
 * 创建日期：2026-08-26
 *============================================================================*/
#include "ai_internal.h"

#include <string.h>
#include <stdio.h>

/*==============================================================================
 * 开局库（棋圣档前几手使用固定强开局，提高下限）
 * 依据 26 种指定开局中的花月/浦月黑必胜开局。
 * 坐标以 15 棋盘天元 (7,7) 为基准；只锁定前几手，其后交回完整搜索。
 *============================================================================*/

typedef struct { int row; int col; } OpeningPoint;

/* 执黑：第 3 手黑 3，取花月/浦月/溪月类强进攻点，依次取合法者 */
static const OpeningPoint OPENING_BLACK3[3] = { {8,8}, {9,8}, {6,9} };
/* 执白：第 2 手白 2，取对花月/浦月的强防守点 */
static const OpeningPoint OPENING_WHITE2[3] = { {6,9}, {9,9}, {8,6} };

int apply_opening(const int grid[BOARD_SIZE][BOARD_SIZE], int player,
                  int moveCount, int *row, int *col)
{
    int i;
    const OpeningPoint *tab;
    int len;

    if (player == CELL_BLACK && moveCount == 2)
    {
        tab = OPENING_BLACK3;
        len = (int)(sizeof(OPENING_BLACK3) / sizeof(OPENING_BLACK3[0]));
    }
    else if (player == CELL_WHITE && moveCount == 1)
    {
        tab = OPENING_WHITE2;
        len = (int)(sizeof(OPENING_WHITE2) / sizeof(OPENING_WHITE2[0]));
    }
    else
    {
        return 0;
    }

    for (i = 0; i < len; i++)
    {
        if (in_range(tab[i].row, tab[i].col) && grid[tab[i].row][tab[i].col] == CELL_EMPTY)
        {
            *row = tab[i].row;
            *col = tab[i].col;
            return 1;
        }
    }
    return 0;
}

/*==============================================================================
 * 阵型识别
 * 判定核心：形状匹配，与 AI 威胁判定同源（复用 pattern_score / point_score）。
 *============================================================================*/

/* 梅花阵：以 player 某子为中心，4 个斜对角 (±2,±2) 布有 ≥3 个同色子 */
int detect_meihua(const int grid[BOARD_SIZE][BOARD_SIZE], int me)
{
    static const int OFF[4][2] = { {2,2}, {2,-2}, {-2,2}, {-2,-2} };
    int row;
    int col;
    for (row = 2; row < BOARD_SIZE - 2; row++)
    {
        for (col = 2; col < BOARD_SIZE - 2; col++)
        {
            int i;
            int cnt = 0;
            if (grid[row][col] != me) continue;
            for (i = 0; i < 4; i++)
            {
                int r = row + OFF[i][0];
                int c = col + OFF[i][1];
                if (in_range(r, c) && grid[r][c] == me) cnt++;
            }
            if (cnt >= 3) return 1;
        }
    }
    return 0;
}

/* 八卦阵：全盘马步（日字）对 ≥ 阈值 */
int count_bagua_pairs(const int grid[BOARD_SIZE][BOARD_SIZE], int me)
{
    static const int KNIGHT[8][2] =
    {
        { -1, -2 }, { -1, 2 }, { 1, -2 }, { 1, 2 },
        { -2, -1 }, { -2, 1 }, { 2, -1 }, { 2, 1 }
    };
    int pairs = 0;
    int row;
    int col;
    for (row = 0; row < BOARD_SIZE; row++)
    {
        for (col = 0; col < BOARD_SIZE; col++)
        {
            int i;
            if (grid[row][col] != me) continue;
            for (i = 0; i < 4; i++)   /* 只数一半避免重复（固定向右/下方向） */
            {
                int r = row + KNIGHT[i][0];
                int c = col + KNIGHT[i][1];
                if (in_range(r, c) && grid[r][c] == me) pairs++;
            }
        }
    }
    return pairs;
}

/* 燕阵：两条不同斜线上各 ≥3 子连续展开。
 * 注意：必须按「独立斜线段」计数。旧实现从每个同色格点都向后数一遍，
 * 一条斜向 4 连（如冲四）会被数出 2~3 条「长度≥3」的伪线段，从而把
 * 单条斜线误判成需要两条斜线的燕阵。改为仅从线段起点开始统计。 */
int has_long_diag(const int grid[BOARD_SIZE][BOARD_SIZE], int me, int *nDiag)
{
    int diagCount = 0;
    int row;
    int col;
    for (row = 0; row < BOARD_SIZE; row++)
    {
        for (col = 0; col < BOARD_SIZE; col++)
        {
            if (grid[row][col] != me) continue;
            /* 主对角线（\）方向：若左上已是同色子，本格不是线段起点，跳过 */
            if (!(row > 0 && col > 0 && grid[row - 1][col - 1] == me))
            {
                int r = row, c = col, len = 0;
                while (in_range(r, c) && grid[r][c] == me) { len++; r++; c++; }
                if (len >= 3) diagCount++;
            }
            /* 副对角线（/）方向：若右上已是同色子，本格不是线段起点，跳过 */
            if (!(row > 0 && col < BOARD_SIZE - 1 && grid[row - 1][col + 1] == me))
            {
                int r = row, c = col, len = 0;
                while (in_range(r, c) && grid[r][c] == me) { len++; r++; c--; }
                if (len >= 3) diagCount++;
            }
        }
    }
    *nDiag = diagCount;
    return (diagCount >= 2);
}

/* 流星阵：同一条斜线 ≥4 子连续进攻 */
int detect_liuxing(const int grid[BOARD_SIZE][BOARD_SIZE], int me)
{
    int row;
    int col;
    for (row = 0; row < BOARD_SIZE; row++)
    {
        for (col = 0; col < BOARD_SIZE; col++)
        {
            if (grid[row][col] != me) continue;
            /* 主对角线 */
            {
                int r = row, c = col, len = 0;
                while (in_range(r, c) && grid[r][c] == me) { len++; r++; c++; }
                if (len >= 4) return 1;
            }
            /* 副对角线 */
            {
                int r = row, c = col, len = 0;
                while (in_range(r, c) && grid[r][c] == me) { len++; r++; c--; }
                if (len >= 4) return 1;
            }
        }
    }
    return 0;
}

/* 斜三阵：三子斜连成角（主/副对角线方向连续 ≥3） */
int detect_xiesan(const int grid[BOARD_SIZE][BOARD_SIZE], int me)
{
    int row;
    int col;
    for (row = 0; row < BOARD_SIZE; row++)
    {
        for (col = 0; col < BOARD_SIZE; col++)
        {
            if (grid[row][col] != me) continue;
            {
                int r = row, c = col, len = 0;
                while (in_range(r, c) && grid[r][c] == me) { len++; r++; c++; }
                if (len >= 3) return 1;
            }
            {
                int r = row, c = col, len = 0;
                while (in_range(r, c) && grid[r][c] == me) { len++; r++; c--; }
                if (len >= 3) return 1;
            }
        }
    }
    return 0;
}

/* 四角阵：四子成正方形（以任一对角线端点为中心，检查 4 角点布有 player） */
int detect_sijiao(const int grid[BOARD_SIZE][BOARD_SIZE], int me)
{
    int row;
    int col;
    for (row = 1; row < BOARD_SIZE - 1; row++)
    {
        for (col = 1; col < BOARD_SIZE - 1; col++)
        {
            if (grid[row - 1][col - 1] == me && grid[row - 1][col + 1] == me &&
                grid[row + 1][col - 1] == me && grid[row + 1][col + 1] == me)
                return 1;
        }
    }
    return 0;
}

/* 十字阵：四子成十字（以某子为中心，上下左右各一子） */
int detect_shizi(const int grid[BOARD_SIZE][BOARD_SIZE], int me)
{
    int row;
    int col;
    for (row = 1; row < BOARD_SIZE - 1; row++)
    {
        for (col = 1; col < BOARD_SIZE - 1; col++)
        {
            if (grid[row][col] != me) continue;
            if (grid[row - 1][col] == me && grid[row + 1][col] == me &&
                grid[row][col - 1] == me && grid[row][col + 1] == me)
                return 1;
        }
    }
    return 0;
}

/* 阵法中文名（供播报显示） */
const char *ai_formation_name(FormationType f)
{
    switch (f)
    {
    case FORMATION_MEIHUA:  return "梅花阵";
    case FORMATION_BAGUA:   return "八卦阵";
    case FORMATION_YAN:     return "燕阵";
    case FORMATION_LIUXING: return "流星阵";
    case FORMATION_XIESAN:  return "斜三阵";
    case FORMATION_SIJIAO:  return "四角阵";
    case FORMATION_SHIZI:   return "十字阵";
    default:                return "无阵法";
    }
}

/* 阵法检测主入口：按优先级检测，返回第一个（最高优先级）成型阵法
 * 优先级说明（2026-08-26 修正）：流星阵（同一斜线 ≥4 连）必须先于燕阵判定。
 * 旧顺序先判燕阵，当盘面存在冲四（斜向 4 连）且另叠一条斜三时，
 * 会把「冲四」误报成「燕阵」（用户反馈）。将流星阵提前后，斜向 4 连
 * 一律按流星阵播报，燕阵仅保留给真正的双斜翼布局。 */
FormationType formation_from_grid(const int grid[BOARD_SIZE][BOARD_SIZE], int player)
{
    int nDiag = 0;
    if (detect_meihua(grid, player))         return FORMATION_MEIHUA;
    if (detect_liuxing(grid, player))        return FORMATION_LIUXING;
    if (has_long_diag(grid, player, &nDiag)) return FORMATION_YAN;
    if (detect_xiesan(grid, player))         return FORMATION_XIESAN;
    if (detect_sijiao(grid, player))         return FORMATION_SIJIAO;
    if (detect_shizi(grid, player))          return FORMATION_SHIZI;
    if (count_bagua_pairs(grid, player) >= 5) return FORMATION_BAGUA;

    return FORMATION_NONE;
}

/* 对外接口：检测当前棋盘上 player 方的成型阵法 */
FormationType ai_detect_formation(const Board *board, int player)
{
    if (board == NULL) return FORMATION_NONE;
    return formation_from_grid(board->grid, player);
}

/* 生成激情解说文本（含阵法名 + 一句话简介 + 棋路特点） */
void ai_formation_comment(const Board *board, int player, char *buf, int size)
{
    FormationType f;
    const char *name;
    const char *line;
    if (buf == NULL || size <= 0) return;
    if (board == NULL) { buf[0] = '\0'; return; }

    f = ai_detect_formation(board, player);
    if (f == FORMATION_NONE)
    {
        buf[0] = '\0';
        return;
    }
    name = ai_formation_name(f);
    line = (player == CELL_BLACK) ? "黑方" : "白方";

    switch (f)
    {
    case FORMATION_MEIHUA:
        snprintf(buf, (size_t)size, "%s %s 成型！中宫开花四角呼应，攻防兼备，牵一发而动全身。",
                 line, name);
        break;
    case FORMATION_BAGUA:
        snprintf(buf, (size_t)size, "%s 布下 %s！马步错落如织网，步步为营，一子落而八方呼应。",
                 line, name);
        break;
    case FORMATION_YAN:
        snprintf(buf, (size_t)size, "%s %s 双翼展开！斜向如飞燕掠水，左右夹击直取中军。",
                 line, name);
        break;
    case FORMATION_LIUXING:
        snprintf(buf, (size_t)size, "%s %s！一线斜贯如彗星扫空，攻势凌厉速战速决。",
                 line, name);
        break;
    case FORMATION_XIESAN:
        snprintf(buf, (size_t)size, "%s %s 起势！三子成角攻守兼备，可化长蛇可变长勾。",
                 line, name);
        break;
    case FORMATION_SIJIAO:
        snprintf(buf, (size_t)size, "%s %s 成形！四子镇四方，夺先反夺先，剑锋所指。",
                 line, name);
        break;
    case FORMATION_SHIZI:
        snprintf(buf, (size_t)size, "%s %s 立！纵横交错中军坐镇，此阵一成大势已定。",
                 line, name);
        break;
    default:
        buf[0] = '\0';
        break;
    }
}

/* 生成阵型事件文案：根据手数自动区分开局/中盘风格
 * 开局(前12手)：偏向布局定式、抢占要点、阵势初成
 * 中盘：偏向攻防转换、阵势成型、攻势凌厉
 */
void ai_formation_event_text(const Board *board, int player, char *buf, int size)
{
    FormationType f;
    const char *side;
    int isOpening;

    if (buf == NULL || size <= 0) return;
    if (board == NULL) { buf[0] = '\0'; return; }

    f = ai_detect_formation(board, player);
    if (f == FORMATION_NONE) { buf[0] = '\0'; return; }

    side = (player == CELL_BLACK) ? "黑方" : "白方";
    isOpening = (board->moveCount <= 12);

    switch (f)
    {
    case FORMATION_MEIHUA:
        if (isOpening)
            snprintf(buf, (size_t)size, "%s 梅花阵初成！开局抢占中宫，四角呼应，此势一成布局占优。", side);
        else
            snprintf(buf, (size_t)size, "%s 梅花阵成型！中宫开花四角呼应，攻防兼备，牵一发而动全身。", side);
        break;
    case FORMATION_BAGUA:
        if (isOpening)
            snprintf(buf, (size_t)size, "%s 八卦阵布局！马步错落如织网，开局布下此阵，步步为营后劲十足。", side);
        else
            snprintf(buf, (size_t)size, "%s 布下八卦阵！马步错落八方呼应，此阵一成，对方处处受制。", side);
        break;
    case FORMATION_YAN:
        if (isOpening)
            snprintf(buf, (size_t)size, "%s 燕阵起势！开局双翼斜展，左右呼应，暗藏夹击之势。", side);
        else
            snprintf(buf, (size_t)size, "%s 燕阵双翼展开！斜向如飞燕掠水，左右夹击直取中军。", side);
        break;
    case FORMATION_LIUXING:
        if (isOpening)
            snprintf(buf, (size_t)size, "%s 流星阵初现！开局一线斜贯，抢占要道，攻势已起。", side);
        else
            snprintf(buf, (size_t)size, "%s 流星阵！一线斜贯如彗星扫空，攻势凌厉速战速决。", side);
        break;
    case FORMATION_XIESAN:
        if (isOpening)
            snprintf(buf, (size_t)size, "%s 斜三阵起手！开局三子成角，可攻可守，变化多端。", side);
        else
            snprintf(buf, (size_t)size, "%s 斜三阵起势！三子成角攻守兼备，可化长蛇可变长勾。", side);
        break;
    case FORMATION_SIJIAO:
        if (isOpening)
            snprintf(buf, (size_t)size, "%s 四角阵布局！开局四子镇四方，控制点面，大势在握。", side);
        else
            snprintf(buf, (size_t)size, "%s 四角阵成形！四子镇四方，夺先反夺先，剑锋所指。", side);
        break;
    case FORMATION_SHIZI:
        if (isOpening)
            snprintf(buf, (size_t)size, "%s 十字阵立！开局纵横交错中军坐镇，此阵一成布局已定。", side);
        else
            snprintf(buf, (size_t)size, "%s 十字阵立！纵横交错中军坐镇，此阵一成大势已定。", side);
        break;
    default:
        buf[0] = '\0';
        break;
    }
}

/* 内部：统计 player 落一子后能成形阵法的候选空点数（基于 grid）。
 * 供 STYLE_TRICK 在 gen_candidates 中叠加阵法候选点加分。 */
int formation_hint_grid(const int grid[BOARD_SIZE][BOARD_SIZE], int player)
{
    int list[MAX_MOVES];
    int count;
    int i;
    int n = 0;

    count = collect_empty_neighbors(grid, list);
    for (i = 0; i < count; i++)
    {
        int row = list[i] / BOARD_SIZE;
        int col = list[i] % BOARD_SIZE;
        int before;
        int after;

        /* 用棋盘副本测落子前后是否从「未成型」变为「成型」 */
        int tmp[BOARD_SIZE][BOARD_SIZE];
        memcpy(tmp, grid, sizeof(tmp));
        tmp[row][col] = player;
        after  = (formation_from_grid((const int (*)[BOARD_SIZE])tmp, player) != FORMATION_NONE);

        memcpy(tmp, grid, sizeof(tmp));   /* 撤回到原状 */
        before = (formation_from_grid((const int (*)[BOARD_SIZE])tmp, player) != FORMATION_NONE);

        /* 原本未成型、落这一子后成型 => 是阵法候选点 */
        if (!before && after) n++;
    }
    return n;
}

/* 对外接口：统计 player 落一子后能成形阵法的候选空点数（0..N） */
int ai_formation_hint(const Board *board, int player)
{
    if (board == NULL) return 0;
    return formation_hint_grid(board->grid, player);
}

/*==============================================================================
 * 获取成型阵法的棋子坐标
 * 根据阵型类型，遍历棋盘找到组成该阵型的核心棋子坐标。
 * 返回棋子数量，0 表示无成型阵法或阵型过于复杂无法提取。
 *============================================================================*/
int ai_formation_cells(const Board *board, int player, int *rows, int *cols, int maxCells)
{
    FormationType f;
    int count = 0;
    int row;
    int col;

    if (board == NULL || rows == NULL || cols == NULL || maxCells <= 0) return 0;

    f = formation_from_grid(board->grid, player);
    if (f == FORMATION_NONE) return 0;

    switch (f)
    {
    case FORMATION_SIJIAO:   /* 四角阵：以某点为中心，4个角点 (r±1,c±1) 都是 player */
        for (row = 1; row < BOARD_SIZE - 1 && count < maxCells; row++)
        {
            for (col = 1; col < BOARD_SIZE - 1 && count < maxCells; col++)
            {
                if (board->grid[row-1][col-1] == player && board->grid[row-1][col+1] == player &&
                    board->grid[row+1][col-1] == player && board->grid[row+1][col+1] == player)
                {
                    rows[count] = row-1; cols[count] = col-1; count++;
                    if (count < maxCells) { rows[count] = row-1; cols[count] = col+1; count++; }
                    if (count < maxCells) { rows[count] = row+1; cols[count] = col-1; count++; }
                    if (count < maxCells) { rows[count] = row+1; cols[count] = col+1; count++; }
                    return count;
                }
            }
        }
        break;

    case FORMATION_SHIZI:   /* 十字阵：中心 + 上下左右都是 player */
        for (row = 1; row < BOARD_SIZE - 1 && count < maxCells; row++)
        {
            for (col = 1; col < BOARD_SIZE - 1 && count < maxCells; col++)
            {
                if (board->grid[row][col] == player &&
                    board->grid[row-1][col] == player && board->grid[row+1][col] == player &&
                    board->grid[row][col-1] == player && board->grid[row][col+1] == player)
                {
                    rows[count] = row; cols[count] = col; count++;
                    if (count < maxCells) { rows[count] = row-1; cols[count] = col; count++; }
                    if (count < maxCells) { rows[count] = row+1; cols[count] = col; count++; }
                    if (count < maxCells) { rows[count] = row; cols[count] = col-1; count++; }
                    if (count < maxCells) { rows[count] = row; cols[count] = col+1; count++; }
                    return count;
                }
            }
        }
        break;

    case FORMATION_MEIHUA:   /* 梅花阵：中心 + 4个斜对角 (r±2,c±2) ≥3个 player */
        for (row = 2; row < BOARD_SIZE - 2 && count < maxCells; row++)
        {
            for (col = 2; col < BOARD_SIZE - 2 && count < maxCells; col++)
            {
                static const int OFF[4][2] = { {2,2}, {2,-2}, {-2,2}, {-2,-2} };
                int cnt = 0;
                int i;
                if (board->grid[row][col] != player) continue;
                for (i = 0; i < 4; i++)
                {
                    int r = row + OFF[i][0];
                    int c = col + OFF[i][1];
                    if (board->grid[r][c] == player) cnt++;
                }
                if (cnt >= 3)
                {
                    rows[count] = row; cols[count] = col; count++;
                    for (i = 0; i < 4 && count < maxCells; i++)
                    {
                        int r = row + OFF[i][0];
                        int c = col + OFF[i][1];
                        if (board->grid[r][c] == player)
                        { rows[count] = r; cols[count] = c; count++; }
                    }
                    return count;
                }
            }
        }
        break;

    case FORMATION_LIUXING:   /* 流星阵：斜线上 ≥4 子连续 */
    case FORMATION_XIESAN:   /* 斜三阵：斜线上 ≥3 子连续 */
    case FORMATION_YAN:       /* 燕阵：两条斜线各 ≥3 子 */
    {
        int minLen = (f == FORMATION_LIUXING) ? 4 : 3;
        int bestLen = 0;
        int bestRow = -1, bestCol = -1, bestDir = 0;
        static const int DIRS[2][2] = { {1,1}, {1,-1} };
        int d;
        for (d = 0; d < 2; d++)
        {
            int dr = DIRS[d][0], dc = DIRS[d][1];
            for (row = 0; row < BOARD_SIZE; row++)
            {
                for (col = 0; col < BOARD_SIZE; col++)
                {
                    int len = 0;
                    int r = row, c = col;
                    if (board->grid[row][col] != player) continue;
                    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE &&
                           board->grid[r][c] == player) { len++; r += dr; c += dc; }
                    if (len >= minLen && len > bestLen)
                    { bestLen = len; bestRow = row; bestCol = col; bestDir = d; }
                }
            }
        }
        if (bestLen > 0)
        {
            int dr = DIRS[bestDir][0], dc = DIRS[bestDir][1];
            int i;
            for (i = 0; i < bestLen && count < maxCells; i++)
            {
                rows[count] = bestRow + i * dr;
                cols[count] = bestCol + i * dc;
                count++;
            }
            /* 燕阵：再找另一条斜线 */
            if (f == FORMATION_YAN && count < maxCells)
            {
                int otherDir = 1 - bestDir;
                int odr = DIRS[otherDir][0], odc = DIRS[otherDir][1];
                int oBest = 0, oRow = -1, oCol = -1;
                for (row = 0; row < BOARD_SIZE; row++)
                {
                    for (col = 0; col < BOARD_SIZE; col++)
                    {
                        int len = 0;
                        int r = row, c = col;
                        if (board->grid[row][col] != player) continue;
                        while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE &&
                               board->grid[r][c] == player) { len++; r += odr; c += odc; }
                        if (len >= 3 && len > oBest) { oBest = len; oRow = row; oCol = col; }
                    }
                }
                if (oBest > 0)
                {
                    int i;
                    for (i = 0; i < oBest && count < maxCells; i++)
                    {
                        rows[count] = oRow + i * odr;
                        cols[count] = oCol + i * odc;
                        count++;
                    }
                }
            }
            return count;
        }
        break;
    }

    case FORMATION_BAGUA:   /* 八卦阵：提取参与马步对的棋子（去重，最多8个） */
    {
        static const int KNIGHT[4][2] =
        {
            { -1, -2 }, { -1, 2 }, { 1, -2 }, { 1, 2 }
        };
        int marked[BOARD_SIZE][BOARD_SIZE];
        int r, c, i;
        memset(marked, 0, sizeof(marked));
        for (row = 0; row < BOARD_SIZE; row++)
        {
            for (col = 0; col < BOARD_SIZE; col++)
            {
                if (board->grid[row][col] != player) continue;
                for (i = 0; i < 4; i++)
                {
                    r = row + KNIGHT[i][0];
                    c = col + KNIGHT[i][1];
                    if (in_range(r, c) && board->grid[r][c] == player)
                    {
                        marked[row][col] = 1;
                        marked[r][c] = 1;
                    }
                }
            }
        }
        for (row = 0; row < BOARD_SIZE && count < maxCells; row++)
        {
            for (col = 0; col < BOARD_SIZE && count < maxCells; col++)
            {
                if (marked[row][col])
                {
                    rows[count] = row; cols[count] = col; count++;
                }
            }
        }
        return count;
    }

    default:
        break;
    }

    return count;
}
