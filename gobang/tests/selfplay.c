/*==============================================================================
 * selfplay.c - 自对弈强度验证(纯 C,无 EGE 依赖)
 * 让两个 AI 档位互相下棋,统计胜率,作为难度梯度递增的验收工具。
 *
 * 用法: selfplay <档位A 0-5> <档位B 0-5> <局数> [风格A] [风格B]
 * 每局黑白轮换(偶数局 A 执黑先手),输出每局结果与胜率统计。
 *
 * Build: gcc -o tests/selfplay.exe tests/selfplay.c src/ai.c src/board.c -Isrc
 * 验收线(见设计文档 v3 第十一章):相邻档位高者胜率 >= 80%
 *============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "board.h"
#include "ai.h"

static const char *color_name(int c)
{
    return (c == CELL_BLACK) ? "黑" : "白";
}

/* 白方常见应手(黑方固定天元),用于开局多样化 */
static const int OPENINGS[12][2] =
{
    { 7, 6 }, { 6, 7 }, { 6, 6 }, { 8, 8 }, { 7, 8 }, { 8, 7 },
    { 6, 8 }, { 8, 6 }, { 5, 5 }, { 9, 9 }, { 5, 7 }, { 7, 5 }
};

int main(int argc, char *argv[])
{
    int levelA;
    int levelB;
    int games;
    int styleA;
    int styleB;
    int i;
    int winsA = 0;
    int winsB = 0;
    int draws = 0;

    if (argc < 4)
    {
        printf("用法: selfplay <档位A 0-5> <档位B 0-5> <局数> [风格A] [风格B]\n");
        return 1;
    }
    levelA = atoi(argv[1]);
    levelB = atoi(argv[2]);
    games  = atoi(argv[3]);
    styleA = (argc > 4) ? atoi(argv[4]) : STYLE_NORMAL;
    styleB = (argc > 5) ? atoi(argv[5]) : STYLE_NORMAL;

    if (levelA < 0 || levelA >= LEVEL_COUNT ||
        levelB < 0 || levelB >= LEVEL_COUNT || games <= 0)
    {
        printf("参数不合法\n");
        return 1;
    }

    ai_init((unsigned int)time(NULL));

    printf("selfplay: A(%s/%s) vs B(%s/%s), %d 局, 黑白轮换\n",
           ai_level_text(levelA), ai_style_text(styleA),
           ai_level_text(levelB), ai_style_text(styleB), games);

    for (i = 0; i < games; i++)
    {
        Board b;
        int aColor = (i % 2 == 0) ? CELL_BLACK : CELL_WHITE;
        int bColor = OPPONENT(aColor);
        int moves  = 2;
        int winner = GAME_DRAW;

        board_init(&b);
        /* 开局库变化:黑方固定天元(7,7),白方从 12 种常见应手中轮换。
         * 搜索类档位是确定性算法,不加开局变化会导致所有对局雷同。 */
        board_place(&b, 7, 7);
        board_place(&b, OPENINGS[i % 12][0], OPENINGS[i % 12][1]);
        while (moves < MAX_MOVES)
        {
            int color = (moves % 2 == 0) ? CELL_BLACK : CELL_WHITE;
            int row = -1;
            int col = -1;
            int ret;
            int level = (color == aColor) ? levelA : levelB;
            int style = (color == aColor) ? styleA : styleB;

            ai_set_style(style);
            ret = ai_choose(&b, color, level, &row, &col);
            if (ret != AI_OK)
            {
                winner = OPPONENT(color);   /* 无处可下:对方胜 */
                break;
            }
            if (board_place(&b, row, col) != PLACE_OK)
            {
                winner = OPPONENT(color);
                break;
            }
            moves++;
            if (board_check_win(&b, row, col, color, NULL) == 1)
            {
                winner = color;
                break;
            }
        }

        if (winner == aColor) winsA++;
        else if (winner == bColor) winsB++;
        else draws++;

        printf("  局 %2d: %s胜 (A 执%s, %d 手)\n",
               i + 1,
               (winner == aColor) ? "A" : (winner == bColor) ? "B" : "平",
               color_name(aColor), moves);
    }

    printf("------------------------------------------------\n");
    printf("A 胜 %d / B 胜 %d / 平 %d  (A 胜率 %.1f%%)\n",
           winsA, winsB, draws, 100.0 * winsA / games);
    return 0;
}
