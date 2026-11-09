/*==============================================================================
 * test_ai.c - Unit tests for the AI engine module (pure C, ASCII).
 *============================================================================*/
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "ai.h"

static int g_total = 0;
static int g_failed = 0;

#define CHECK(cond, name)                                                   \
    do {                                                                    \
        g_total++;                                                          \
        if (cond) { printf("  [PASS] %s\n", name); }                        \
        else { g_failed++;                                                  \
               printf("  [FAIL] %s   (%s:%d)\n", name, __FILE__, __LINE__); } \
    } while (0)

static void set_four_row(Board *b, int player)
{
    int i;
    board_init(b);
    for (i = 0; i < 4; i++)
    {
        b->grid[7][3 + i] = player;
        b->moves[b->moveCount].row = 7;
        b->moves[b->moveCount].col = 3 + i;
        b->moves[b->moveCount].player = player;
        b->moveCount++;
    }
    b->current = player;
}

static void test_first_move(void)
{
    Board b;
    int row = -1, col = -1;
    printf("T1 first move on empty board\n");
    board_init(&b);
    CHECK(ai_choose(&b, CELL_BLACK, LEVEL_NORMAL, &row, &col) == AI_OK, "empty board returns a move");
    CHECK(row == BOARD_SIZE / 2 && col == BOARD_SIZE / 2, "first move is the center (7,7)");
}

static void test_take_win(void)
{
    Board b;
    int row = -1, col = -1;
    printf("T2 take immediate win\n");
    set_four_row(&b, CELL_BLACK);
    CHECK(ai_choose(&b, CELL_BLACK, LEVEL_EASY, &row, &col) == AI_OK, "returns a move");
    CHECK(row == 7 && (col == 2 || col == 7), "winning move is at an open end of the four");
}

static void test_block_five(void)
{
    Board b;
    int row = -1, col = -1;
    printf("T3 block opponent five\n");
    set_four_row(&b, CELL_WHITE);
    b.current = CELL_BLACK;
    CHECK(ai_choose(&b, CELL_BLACK, LEVEL_EASY, &row, &col) == AI_OK, "returns a move");
    CHECK(row == 7 && (col == 2 || col == 7), "blocking move is at an open end of white's four");
}

static void test_levels_legal(void)
{
    static const int LEVELS[] = { LEVEL_EASY, LEVEL_NORMAL, LEVEL_HARD };
    int i;
    Board b;
    printf("T4 legal move at all difficulty levels\n");
    board_init(&b);
    board_place(&b, 7, 7);
    for (i = 0; i < 3; i++)
    {
        int row = -1, col = -1;
        int ret = ai_choose(&b, CELL_WHITE, LEVELS[i], &row, &col);
        CHECK(ret == AI_OK && board_in_range(row, col) && b.grid[row][col] == CELL_EMPTY, "legal move returned");
    }
}

static void test_full_board(void)
{
    Board b;
    int r, c;
    int row = -1, col = -1;
    printf("T5 full board\n");
    board_init(&b);
    for (r = 0; r < BOARD_SIZE; r++)
        for (c = 0; c < BOARD_SIZE; c++)
        {
            b.grid[r][c] = (((c / 2) % 2) == 0) ? CELL_BLACK : CELL_WHITE;
            b.moves[b.moveCount].row = r;
            b.moves[b.moveCount].col = c;
            b.moves[b.moveCount].player = b.grid[r][c];
            b.moveCount++;
        }
    CHECK(ai_choose(&b, CELL_BLACK, LEVEL_NORMAL, &row, &col) == AI_ERR_NO_MOVE, "full board returns NO_MOVE");
}

static void test_time_limit(void)
{
    Board b;
    int row = -1, col = -1;
    int ret;
    printf("T6 hard-level time limit\n");
    board_init(&b);
    board_place(&b, 7, 7); board_place(&b, 8, 8); board_place(&b, 6, 8);
    board_place(&b, 8, 6); board_place(&b, 6, 6);
    ret = ai_choose(&b, CELL_WHITE, LEVEL_HARD, &row, &col);
    CHECK(ret == AI_OK, "hard level returns a move");
    CHECK(ai_last_time_ms() <= 2000, "single move within 2 seconds");
    printf("      hard level took %d ms, depth %d\n", ai_last_time_ms(), ai_last_depth());
}

static void test_evaluate(void)
{
    Board b;
    int before, after;
    printf("T7 evaluation monotonicity\n");
    board_init(&b);
    before = ai_evaluate(&b, CELL_BLACK);
    set_four_row(&b, CELL_BLACK);
    b.current = CELL_BLACK;
    after = ai_evaluate(&b, CELL_BLACK);
    CHECK(after > before, "evaluation rises after forming four");
}

static void test_master_defense(void)
{
    Board b;
    int row = -1, col = -1;
    printf("T8 master blocks opponent open four (VCF must not override)\n");
    set_four_row(&b, CELL_WHITE);
    b.current = CELL_BLACK;
    CHECK(ai_choose(&b, CELL_BLACK, LEVEL_MASTER, &row, &col) == AI_OK, "returns a move");
    CHECK(row == 7 && (col == 2 || col == 7), "must block white's open four");
}

static void test_master_double_three(void)
{
    Board b;
    int row = -1, col = -1;
    printf("T9 master takes double-three winning point\n");
    board_init(&b);
    /* 黑双活三:横 (7,4)(7,5)(7,6) + 竖 (4,7)(5,7)(6,7),交点 (7,7) 空 */
    b.grid[7][4] = b.grid[7][5] = b.grid[7][6] = CELL_BLACK;
    b.grid[4][7] = b.grid[5][7] = b.grid[6][7] = CELL_BLACK;
    b.grid[0][0] = CELL_WHITE;
    b.grid[14][14] = CELL_WHITE;
    b.moveCount = 8;
    b.current = CELL_BLACK;
    CHECK(ai_choose(&b, CELL_BLACK, LEVEL_MASTER, &row, &col) == AI_OK, "returns a move");
    CHECK(row == 7 && col == 7, "takes the double-four point (7,7)");
}

static void test_grandmaster_defense(void)
{
    Board b;
    int row = -1, col = -1;
    printf("T10 grandmaster blocks opponent open four (PVS/VCT must not override)\n");
    set_four_row(&b, CELL_WHITE);
    b.current = CELL_BLACK;
    CHECK(ai_choose(&b, CELL_BLACK, LEVEL_GRANDMASTER, &row, &col) == AI_OK, "returns a move");
    CHECK(row == 7 && (col == 2 || col == 7), "must block white's open four");
}

int main(void)
{
    printf("================================================\n");
    printf("  AI engine module unit test\n");
    printf("================================================\n\n");
    ai_init(20260825u);
    test_first_move(); printf("\n");
    test_take_win(); printf("\n");
    test_block_five(); printf("\n");
    test_levels_legal(); printf("\n");
    test_full_board(); printf("\n");
    test_time_limit(); printf("\n");
    test_evaluate(); printf("\n");
    test_master_defense(); printf("\n");
    test_master_double_three(); printf("\n");
    test_grandmaster_defense(); printf("\n");
    printf("================================================\n");
    printf("  Total %d, passed %d, failed %d\n", g_total, g_total - g_failed, g_failed);
    printf("================================================\n");
    return (g_failed == 0) ? 0 : 1;
}
