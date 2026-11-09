/*==============================================================================
 * probe_normal.c - 复现"普通(minimax)"在第12手不堵黑活三的问题
 * Build: gcc -o tests/probe_normal.exe tests/probe_normal.c src/ai.c src/ai_eval.c
 *        src/ai_search.c src/ai_vcf.c src/ai_formation.c src/board.c src/common.c -Isrc
 *============================================================================*/
#include <stdio.h>
#include "board.h"
#include "ai.h"
#include "ai_internal.h"

/* 对局到第11手：黑(简单)vs白(普通) 的局面 */
static void build(Board *b)
{
    static const int mv[][3] = {
        {7,7,1},{8,8,2},{8,7,1},{9,7,2},{7,9,1},
        {7,8,2},{6,8,1},{9,8,2},{8,6,1},{10,8,2},{11,8,1}
    };
    int i;
    board_init(b);
    for (i = 0; i < 11; i++) board_place(b, mv[i][0], mv[i][1]);
    b->current = CELL_WHITE;   /* 白(普通)走第12手 */
}

int main(void)
{
    Board b;
    int g[BOARD_SIZE][BOARD_SIZE];
    int row = -1, col = -1;
    int i, j;
    AiCandidate cand[AI_MAX_CANDIDATE];
    int cnt;

    ai_init(20260826u);
    build(&b);
    for (i = 0; i < BOARD_SIZE; i++)
        for (j = 0; j < BOARD_SIZE; j++)
            g[i][j] = b.grid[i][j];

    printf("黑斜线威胁: (6,8)(7,7)(8,6) 活三, 两端 (5,9)/(9,5)\n");
    printf("minimax(白,深度3) 走法: ");
    minimax_search(g, 3, CELL_WHITE, CELL_WHITE, 8, &row, &col);
    printf("(%d,%d)\n", row, col);

    /* 打印白方前 12 候选及其得分 */
    cnt = gen_candidates(g, CELL_WHITE, cand, 12);
    printf("白方候选(分点分,分=白*2+黑*1):\n");
    for (i = 0; i < cnt; i++)
    {
        int atk = point_score(g, cand[i].row, cand[i].col, CELL_WHITE);
        int def = point_score(g, cand[i].row, cand[i].col, CELL_BLACK);
        printf("  (%d,%d) atk=%d def=%d score=%d%s\n",
               cand[i].row, cand[i].col, atk, def, cand[i].score,
               ((cand[i].row==5&&cand[i].col==9)||(cand[i].row==9&&cand[i].col==5)) ? "  <-- 堵活三端" : "");
    }
    return 0;
}
