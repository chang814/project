/*==============================================================================
 * probe_depth.c - 实测各档位在中盘局面的耗时/深度/落点（纯C，无EGE）
 * 用法: probe_depth
 * Build: gcc -o tests/probe_depth.exe tests/probe_depth.c src/ai.c src/ai_eval.c
 *        src/ai_search.c src/ai_vcf.c src/ai_formation.c src/board.c src/common.c -Isrc
 *============================================================================*/
#include <stdio.h>
#include "board.h"
#include "ai.h"
#include "ai_internal.h"

static int has_immediate_win(const Board *b, int player)
{
    int r, c;
    for (r = 0; r < BOARD_SIZE; r++)
    {
        for (c = 0; c < BOARD_SIZE; c++)
        {
            Board tmp = *b;
            if (tmp.grid[r][c] != CELL_EMPTY) continue;
            tmp.grid[r][c] = player;
            if (board_check_win(&tmp, r, c, player, NULL) == 1) return 1;
        }
    }
    return 0;
}

static void build_mid(Board *b)
{
    /* 分散中盘：黑 8 子 / 白 8 子，无立即成五、无一步活四 */
    static const int mv[][3] = {
        {7,7,1},{8,8,2},{5,8,1},{6,6,2},{9,6,1},
        {10,8,2},{8,4,1},{4,6,2},{6,10,1},{7,5,2},
        {10,5,1},{5,10,2},{4,9,1},{9,3,2},{7,3,1},
        {6,9,2}
    };
    int i;
    board_init(b);
    for (i = 0; i < 16; i++)
    {
        board_place(b, mv[i][0], mv[i][1]);
    }
    b->current = CELL_BLACK;
}

int main(void)
{
    int level;
    Board mid;
    printf("level | 颜色 | 耗时ms | 深度 | 落点\n");
    ai_init(20260826u);
    build_mid(&mid);
    printf("局检: 黑立即成五=%d, 白立即成五=%d\n",
           has_immediate_win(&mid, CELL_BLACK),
           has_immediate_win(&mid, CELL_WHITE));
    {
        int g[BOARD_SIZE][BOARD_SIZE];
        int r = -1, c = -1;
        int i, j;
        for (i = 0; i < BOARD_SIZE; i++)
            for (j = 0; j < BOARD_SIZE; j++)
                g[i][j] = mid.grid[i][j];
        printf("黑 detect_vcf=%d (move=%d,%d), detect_vct=%d (move=%d,%d)\n",
               detect_vcf(g, CELL_BLACK, &r, &c), r, c,
               detect_vct(g, CELL_BLACK, &r, &c), r, c);
    }
    for (level = LEVEL_BEGINNER; level < LEVEL_COUNT; level++)
    {
        Board b = mid;
        int row = -1, col = -1;
        int ret;
        ret = ai_choose(&b, CELL_BLACK, level, &row, &col);
        printf("  %2d  | 黑  |  %4d  |  %2d  | (%d,%d) ret=%d\n",
               level, ai_last_time_ms(), ai_last_depth(), row, col, ret);
    }
    return 0;
}
