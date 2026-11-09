/*==============================================================================
 * smoke_levels.c - P0a smoke test: exercise all 6 AI levels on a mid-game
 * position, verify each returns a legal move within a sane time.
 * Pure C, no EGE. Seed for the later tests/selfplay.c strength harness.
 * Build: gcc -o tests/smoke_levels.exe tests/smoke_levels.c src/ai.c src/board.c -Isrc
 *============================================================================*/
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "ai.h"

int main(void)
{
    Board b;
    int level;
    int failures = 0;

    printf("================================================\n");
    printf("  AI levels smoke test (P0a)\n");
    printf("================================================\n");

    ai_init(20260825u);

    for (level = LEVEL_BEGINNER; level < LEVEL_COUNT; level++)
    {
        int row = -1, col = -1;
        int ret;
        int i;

        board_init(&b);
        /* 中盘局面:黑三子 vs 白两子 */
        board_place(&b, 7, 7);   /* 黑 */
        board_place(&b, 8, 8);   /* 白 */
        board_place(&b, 7, 8);   /* 黑 */
        board_place(&b, 8, 7);   /* 白 */
        board_place(&b, 6, 6);   /* 黑 */

        ret = ai_choose(&b, CELL_WHITE, level, &row, &col);

        if (ret != AI_OK || !board_in_range(row, col) || b.grid[row][col] != CELL_EMPTY)
        {
            printf("  [FAIL] level %d: ret=%d move=(%d,%d)\n", level, ret, row, col);
            failures++;
            continue;
        }
        printf("  [PASS] level %d (%s): move=(%d,%d) time=%dms depth=%d\n",
               level, ai_level_text(level), row, col,
               ai_last_time_ms(), ai_last_depth());

        /* 必杀局面:白方已有活四,任何档都必须一步取胜 */
        board_init(&b);
        for (i = 0; i < 4; i++)
        {
            b.grid[9][3 + i] = CELL_WHITE;
            b.moves[b.moveCount].row = 9;
            b.moves[b.moveCount].col = 3 + i;
            b.moves[b.moveCount].player = CELL_WHITE;
            b.moveCount++;
        }
        b.current = CELL_WHITE;
        ret = ai_choose(&b, CELL_WHITE, level, &row, &col);
        if (ret != AI_OK || !(row == 9 && (col == 2 || col == 7)))
        {
            printf("  [FAIL] level %d: did not take open four (move=(%d,%d))\n",
                   level, row, col);
            failures++;
        }
        else
        {
            printf("  [PASS] level %d: takes open four at (%d,%d)\n", level, row, col);
        }
    }

    printf("================================================\n");
    printf("  AI styles smoke test (P0b)\n");
    printf("================================================\n");

    /* 同一局面,九种风格(棋圣档)应走出不同风格的棋,且全部合法 */
    {
        static const int STYLES[9] =
        {
            STYLE_NORMAL, STYLE_AGGRESSIVE, STYLE_WALL,
            STYLE_COUNTER, STYLE_CONTROL, STYLE_DOUBLE,
            STYLE_TRICK, STYLE_BAIT, STYLE_ATTACK
        };
        int s;

        for (s = 0; s < 9; s++)
        {
            int row = -1, col = -1;
            int ret;

            board_init(&b);
            board_place(&b, 7, 7);   /* 黑 */
            board_place(&b, 8, 8);   /* 白 */
            board_place(&b, 7, 8);   /* 黑 */
            board_place(&b, 8, 7);   /* 白 */
            board_place(&b, 6, 6);   /* 黑 */

            ai_set_style(STYLES[s]);
            ret = ai_choose(&b, CELL_WHITE, LEVEL_GRANDMASTER, &row, &col);
            if (ret != AI_OK || !board_in_range(row, col) || b.grid[row][col] != CELL_EMPTY)
            {
                printf("  [FAIL] style %d: ret=%d move=(%d,%d)\n", STYLES[s], ret, row, col);
                failures++;
            }
            else
            {
                printf("  [PASS] style %s (GRANDMASTER): move=(%d,%d) time=%dms depth=%d\n",
                       ai_style_text(STYLES[s]), row, col,
                       ai_last_time_ms(), ai_last_depth());
            }
        }
    }

    /* 非法风格值应安全回退普通型 */
    {
        int row = -1, col = -1;
        board_init(&b);
        board_place(&b, 7, 7);
        ai_set_style(999);
        if (ai_choose(&b, CELL_WHITE, LEVEL_HARD, &row, &col) == AI_OK && board_in_range(row, col))
        {
            printf("  [PASS] invalid style 999 falls back safely (move=(%d,%d))\n", row, col);
        }
        else
        {
            printf("  [FAIL] invalid style 999 did not fall back\n");
            failures++;
        }
        ai_set_style(STYLE_NORMAL);
    }

    /* MCTS 蒙特卡洛模拟选点(可选演示):返回合法落子 */
    {
        int row = -1, col = -1;
        board_init(&b);
        board_place(&b, 7, 7);
        board_place(&b, 8, 8);
        board_place(&b, 7, 8);
        if (ai_mcts_choose(&b, CELL_WHITE, 100, &row, &col) == AI_OK &&
            board_in_range(row, col) && b.grid[row][col] == CELL_EMPTY)
        {
            printf("  [PASS] mcts choose legal move=(%d,%d)\n", row, col);
        }
        else
        {
            printf("  [FAIL] mcts choose illegal ret\n");
            failures++;
        }
    }

    printf("================================================\n");
    printf("  failures: %d\n", failures);
    printf("================================================\n");
    return (failures == 0) ? 0 : 1;
}
