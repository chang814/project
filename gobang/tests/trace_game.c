/*==============================================================================
 * trace_game.c - 打印简单(贪心) vs 普通(minimax2) 的对局过程
 * Build: gcc -o tests/trace_game.exe tests/trace_game.c src/ai.c src/ai_eval.c
 *        src/ai_search.c src/ai_vcf.c src/ai_formation.c src/board.c src/common.c -Isrc
 *============================================================================*/
#include <stdio.h>
#include "board.h"
#include "ai.h"

static const char *color_name(int c) { return (c == CELL_BLACK) ? "黑" : "白"; }

int main(int argc, char *argv[])
{
    Board b;
    int levelA = 1;   /* 简单 */
    int levelB = 2;   /* 普通 */
    int aColor = CELL_BLACK;
    int bColor = CELL_WHITE;
    int moves = 0;
    int winner = GAME_DRAW;
    int i;

    ai_init(20260826u);
    board_init(&b);
    board_place(&b, 7, 7);
    board_place(&b, 8, 8);
    moves = 2;
    printf("开局: 黑(7,7) 白(8,8)\n");

    while (moves < MAX_MOVES)
    {
        int color = (moves % 2 == 0) ? CELL_BLACK : CELL_WHITE;
        int level = (color == aColor) ? levelA : levelB;
        int row = -1, col = -1;
        int ret = ai_choose(&b, color, level, &row, &col);
        if (ret != AI_OK) { winner = OPPONENT(color); printf("  无处可下 %s 胜\n", color_name(winner)); break; }
        board_place(&b, row, col);
        moves++;
        printf("  %2d  %s(%s) 落子(%d,%d)  time=%dms depth=%d\n",
               moves, color_name(color),
               (level == 2) ? "普通" : "简单",
               row, col, ai_last_time_ms(), ai_last_depth());
        if (board_check_win(&b, row, col, color, NULL) == 1)
        {
            winner = color;
            printf("  %s 胜（%d 手）\n", color_name(color), moves);
            break;
        }
    }

    if (winner == GAME_DRAW)
    {
        printf("  平局\n");
        for (i = 0; i < BOARD_SIZE; i++)
        {
            int j;
            for (j = 0; j < BOARD_SIZE; j++)
                printf("%c", (b.grid[i][j] == CELL_BLACK) ? 'X' :
                             (b.grid[i][j] == CELL_WHITE) ? 'O' : '.');
            printf("\n");
        }
    }
    return 0;
}
