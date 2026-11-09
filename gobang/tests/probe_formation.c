/*==============================================================================
 * probe_formation.c - 验证"冲四不再被误判为燕阵"
 * Build: gcc -o tests/probe_formation.exe tests/probe_formation.c src/ai.c
 *        src/ai_eval.c src/ai_search.c src/ai_vcf.c src/ai_formation.c
 *        src/board.c src/common.c -Isrc
 *============================================================================*/
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "ai.h"
#include "ai_internal.h"

static void board_from(Board *b, const int moves[][3], int n)
{
    int i;
    board_init(b);
    for (i = 0; i < n; i++)
    {
        b->current = moves[i][2];   /* board_place 使用 board->current 作为落子方 */
        board_place(b, moves[i][0], moves[i][1]);
    }
}

int main(void)
{
    Board b;
    const char *nm;
    FormationType f;

    /* 场景1：黑方斜向冲四（4连，(3,3)-(6,6)，一端被白堵）+ 另一条黑斜三(8,2)(9,3)(10,4)
     * 预期：流星阵（旧版误判为燕阵） */
    {
        static const int mv[][3] = {
            {3,3,1},{4,4,1},{5,5,1},{6,6,1},{2,2,2},          /* 黑冲四(端堵), 白(2,2) */
            {8,2,1},{9,3,1},{10,4,1}                          /* 黑另斜三 */
        };
        board_from(&b, mv, 8);
        f = ai_detect_formation(&b, CELL_BLACK);
        nm = ai_formation_name(f);
        printf("[场景1] 冲四+斜三  ->  %s   %s\n", nm, (f == FORMATION_LIUXING) ? "OK(流星阵)" :
              (f == FORMATION_YAN ? "!!仍误判燕阵" : "??"));
    }

    /* 场景2：两条斜三(无4连) -> 燕阵 */
    {
        static const int mv[][3] = {
            {5,5,1},{6,6,1},{7,7,1},                          /* 斜三一 */
            {5,9,1},{6,8,1},{7,7,1},                          /* 与中心共用? 改用独立 */
        };
        (void)mv;
    }
    {
        static const int mv[][3] = {
            {4,4,1},{5,5,1},{6,6,1},                          /* 斜三一 (主对角线) */
            {8,2,1},{9,3,1},{10,4,1}                          /* 斜三二 (主对角线,独立) */
        };
        board_from(&b, mv, 6);
        f = ai_detect_formation(&b, CELL_BLACK);
        nm = ai_formation_name(f);
        printf("[场景2] 两斜三    ->  %s   %s\n", nm, (f == FORMATION_YAN) ? "OK(燕阵)" : "??");
    }

    /* 场景3：单独斜向冲四 -> 流星阵 */
    {
        static const int mv[][3] = {
            {3,3,1},{4,4,1},{5,5,1},{6,6,1},{2,2,2}
        };
        board_from(&b, mv, 5);
        f = ai_detect_formation(&b, CELL_BLACK);
        nm = ai_formation_name(f);
        printf("[场景3] 单独冲四  ->  %s   %s\n", nm, (f == FORMATION_LIUXING) ? "OK(流星阵)" : "??");
    }

    /* 场景4：真正双翼燕阵（两条对称斜线各3）-> 燕阵
     * 以(7,7)为中心：主对角线 (5,5)(6,6)(7,7) + 副对角线 (5,9)(6,8)(7,7) */
    {
        static const int mv[][3] = {
            {5,5,1},{6,6,1},{7,7,1},                          /* 一翼 */
            {5,9,1},{6,8,1}                                   /* 另一翼(与中心共) */
        };
        board_from(&b, mv, 5);
        f = ai_detect_formation(&b, CELL_BLACK);
        nm = ai_formation_name(f);
        printf("[场景4] 双翼燕阵  ->  %s   %s\n", nm, (f == FORMATION_YAN) ? "OK(燕阵)" : "??");
    }

    return 0;
}
