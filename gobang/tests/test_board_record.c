/*==============================================================================
 * test_board_record.c - Unit tests for board & record modules (pure C, ASCII).
 *============================================================================*/
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "record.h"

static int g_total = 0;
static int g_failed = 0;

#define CHECK(cond, name)                                                   \
    do {                                                                    \
        g_total++;                                                          \
        if (cond) { printf("  [PASS] %s\n", name); }                        \
        else { g_failed++;                                                  \
               printf("  [FAIL] %s   (%s:%d)\n", name, __FILE__, __LINE__); } \
    } while (0)

static void set_win(Board *b, int player, int winLine[WIN_COUNT][2])
{
    int i;
    board_init(b);
    for (i = 0; i < WIN_COUNT; i++)
    {
        int r = winLine[i][0];
        int c = winLine[i][1];
        b->grid[r][c] = player;
        b->moves[b->moveCount].row = r;
        b->moves[b->moveCount].col = c;
        b->moves[b->moveCount].player = player;
        b->moveCount++;
    }
    b->current = OPPONENT(player);
}

static void test_init_and_legality(void)
{
    Board b;
    int i;

    printf("T1 init and placement legality\n");
    board_init(&b);
    CHECK(b.moveCount == 0, "moveCount is 0 after init");
    CHECK(b.current == CELL_BLACK, "black moves first");
    CHECK(b.status == GAME_PLAYING, "status is playing");

    i = 0;
    while (i < BOARD_SIZE * BOARD_SIZE && b.grid[i / BOARD_SIZE][i % BOARD_SIZE] == CELL_EMPTY) i++;
    CHECK(i == BOARD_SIZE * BOARD_SIZE, "board is empty after init");

    CHECK(board_can_place(&b, -1, 0) == PLACE_ERR_RANGE, "row -1 out of range");
    CHECK(board_can_place(&b, 0, 15) == PLACE_ERR_RANGE, "col 15 out of range");
    CHECK(board_place(&b, 7, 7) == PLACE_OK, "center placement ok");
    CHECK(board_can_place(&b, 7, 7) == PLACE_ERR_OCCUPIED, "occupied point rejected");
    CHECK(b.moveCount == 1 && b.current == CELL_WHITE, "turn switches to white");
    CHECK(board_can_place(NULL, 0, 0) == PLACE_ERR_NULL, "NULL board rejected");
}

static void test_win_four_directions(void)
{
    Board b;
    int line[WIN_COUNT][2];
    int win[WIN_COUNT][2];
    int i;

    printf("T2 win detection in four directions\n");

    for (i = 0; i < WIN_COUNT; i++) { win[i][0] = 7; win[i][1] = 3 + i; }
    set_win(&b, CELL_BLACK, win);
    CHECK(board_check_win(&b, 7, 5, CELL_BLACK, line) == 1, "horizontal five wins");
    CHECK(line[0][1] == 3 && line[4][1] == 7, "horizontal win line correct");

    for (i = 0; i < WIN_COUNT; i++) { win[i][0] = 2 + i; win[i][1] = 5; }
    set_win(&b, CELL_BLACK, win);
    CHECK(board_check_win(&b, 4, 5, CELL_BLACK, NULL) == 1, "vertical five wins");

    for (i = 0; i < WIN_COUNT; i++) { win[i][0] = 3 + i; win[i][1] = 3 + i; }
    set_win(&b, CELL_BLACK, win);
    CHECK(board_check_win(&b, 5, 5, CELL_BLACK, NULL) == 1, "main diagonal wins");

    for (i = 0; i < WIN_COUNT; i++) { win[i][0] = 3 + i; win[i][1] = 7 - i; }
    set_win(&b, CELL_BLACK, win);
    CHECK(board_check_win(&b, 5, 5, CELL_BLACK, NULL) == 1, "anti diagonal wins");

    for (i = 0; i < WIN_COUNT; i++) { win[i][0] = 8; win[i][1] = i; }
    set_win(&b, CELL_WHITE, win);
    CHECK(board_check_win(&b, 8, 2, CELL_WHITE, line) == 1, "white five wins");
    CHECK(b.status == GAME_PLAYING, "check_win does not modify status");

    board_init(&b);
    b.grid[7][3] = CELL_BLACK; b.grid[7][4] = CELL_BLACK;
    b.grid[7][5] = CELL_BLACK; b.grid[7][6] = CELL_BLACK;
    CHECK(board_check_win(&b, 7, 4, CELL_BLACK, NULL) == 0, "four in a row does not win");
}

static void test_undo_and_draw(void)
{
    Board b;
    int r, c;

    printf("T3 undo and draw\n");
    board_init(&b);
    board_place(&b, 7, 7);
    board_place(&b, 8, 8);
    CHECK(b.moveCount == 2, "moveCount is 2");
    CHECK(board_undo(&b) == 1, "undo one step ok");
    CHECK(b.moveCount == 1 && b.current == CELL_WHITE, "undo restores count and turn");
    CHECK(board_get(&b, 8, 8) == CELL_EMPTY, "undone point is empty");
    CHECK(board_undo_steps(&b, 2) == 1, "undo steps limited by stack");
    CHECK(b.moveCount == 0, "stack empty after undo");
    CHECK(board_undo(&b) == 0, "undo on empty stack returns 0");

    board_init(&b);
    for (r = 0; r < BOARD_SIZE; r++)
        for (c = 0; c < BOARD_SIZE; c++)
        {
            int color = (((c / 2) % 2) == 0) ? CELL_BLACK : CELL_WHITE;
            b.grid[r][c] = color;
            b.moves[b.moveCount].row = r;
            b.moves[b.moveCount].col = c;
            b.moves[b.moveCount].player = color;
            b.moveCount++;
        }
    b.current = CELL_BLACK;
    b.status = GAME_PLAYING;
    CHECK(board_is_full(&b) == 1, "full board detected");
    CHECK(board_check_win(&b, 14, 14, CELL_BLACK, NULL) == 0, "full board without five does not win");
}

static void test_record_roundtrip(void)
{
    Board b, loaded;
    GameInfo info, loadedInfo;
    const char *path = "tests/_tmp_save.txt";
    char buf[8];
    int ret;

    printf("T4 record save and load roundtrip\n");
    board_init(&b);
    info.mode = MODE_PVE;
    info.level = LEVEL_HARD;
    info.humanColor = CELL_BLACK;

    board_place(&b, 7, 7);
    board_place(&b, 8, 8);
    board_place(&b, 9, 9);

    ret = record_save(&b, &info, path);
    CHECK(ret == RECORD_OK, "save returns OK");

    memset(&loaded, 0, sizeof(loaded));
    memset(&loadedInfo, 0, sizeof(loadedInfo));
    ret = record_load(&loaded, &loadedInfo, path);
    CHECK(ret == RECORD_OK, "load returns OK");
    CHECK(loaded.moveCount == 3, "loaded moveCount matches");
    CHECK(loadedInfo.mode == MODE_PVE && loadedInfo.level == LEVEL_HARD &&
          loadedInfo.humanColor == CELL_BLACK, "loaded game info matches");
    CHECK(loaded.grid[7][7] == CELL_BLACK && loaded.grid[8][8] == CELL_WHITE &&
          loaded.grid[9][9] == CELL_BLACK, "loaded board stones match");
    CHECK(loaded.current == CELL_WHITE, "loaded turn is white");

    record_coord_to_text(0, 0, buf, (int)sizeof(buf));
    CHECK(strcmp(buf, "A1") == 0, "(0,0) -> A1");
    record_coord_to_text(7, 7, buf, (int)sizeof(buf));
    CHECK(strcmp(buf, "H8") == 0, "(7,7) -> H8");
}

static void test_record_invalid(void)
{
    Board b;
    GameInfo info;
    FILE *fp;
    const char *path = "tests/_tmp_invalid.txt";
    int ret;

    printf("T5 record invalid file defense\n");

    ret = record_load(&b, &info, "tests/_not_exist_xyz.txt");
    CHECK(ret == RECORD_ERR_OPEN, "missing file returns OPEN");

    fp = fopen(path, "w");
    fprintf(fp, "NOT-A-RECORD 1\nEND\n");
    fclose(fp);
    ret = record_load(&b, &info, path);
    CHECK(ret == RECORD_ERR_FORMAT, "bad header returns FORMAT");

    fp = fopen(path, "w");
    fprintf(fp, "%s %d\nMODE 0\nLEVEL 1\nHUMAN 1\nSTATUS 0\nCOUNT 2\n7 7 1\n8 8 1\nEND\n", RECORD_MAGIC, RECORD_VERSION);
    fclose(fp);
    ret = record_load(&b, &info, path);
    CHECK(ret == RECORD_ERR_DATA, "turn mismatch returns DATA");

    fp = fopen(path, "w");
    fprintf(fp, "%s %d\nMODE 0\nLEVEL 1\nHUMAN 1\nSTATUS 0\nCOUNT 1\n20 20 1\nEND\n", RECORD_MAGIC, RECORD_VERSION);
    fclose(fp);
    ret = record_load(&b, &info, path);
    CHECK(ret == RECORD_ERR_DATA, "out-of-range move returns DATA");

    ret = record_save(NULL, &info, path);
    CHECK(ret == RECORD_ERR_NULL, "save NULL returns NULL");
    ret = record_load(NULL, &info, path);
    CHECK(ret == RECORD_ERR_NULL, "load NULL returns NULL");
}

/* T6: STYLE 字段存档往返 + 旧格式(无 STYLE 行)向后兼容 */
static void test_record_style(void)
{
    Board b, loaded;
    GameInfo info, loadedInfo;
    const char *path = "tests/_tmp_style.txt";
    int ret;

    printf("T6 record style roundtrip and backward compatibility\n");

    board_init(&b);
    info.mode = MODE_PVE;
    info.level = LEVEL_HARD;
    info.humanColor = CELL_BLACK;
    info.style = STYLE_AGGRESSIVE;
    board_place(&b, 7, 7);
    board_place(&b, 8, 8);

    ret = record_save(&b, &info, path);
    CHECK(ret == RECORD_OK, "save with STYLE returns OK");

    memset(&loaded, 0, sizeof(loaded));
    memset(&loadedInfo, 0, sizeof(loadedInfo));
    ret = record_load(&loaded, &loadedInfo, path);
    CHECK(ret == RECORD_OK, "load with STYLE returns OK");
    CHECK(loadedInfo.style == STYLE_AGGRESSIVE, "loaded style matches saved style");

    /* 旧格式存档(无 STYLE 行)应默认普通型载入 */
    {
        FILE *fp = fopen(path, "w");
        if (fp != NULL)
        {
            fprintf(fp, "%s %d\nMODE 1\nLEVEL 4\nHUMAN 2\nSTATUS 0\nCOUNT 1\n7 7 1\nEND\n",
                    RECORD_MAGIC, RECORD_VERSION);
            fclose(fp);
        }
    }
    memset(&loadedInfo, 0, sizeof(loadedInfo));
    ret = record_load(&loaded, &loadedInfo, path);
    CHECK(ret == RECORD_OK, "legacy file (no STYLE) loads OK");
    CHECK(loadedInfo.style == STYLE_NORMAL, "legacy file defaults to STYLE_NORMAL");
}

int main(void)
{
    printf("================================================\n");
    printf("  Board & Record module unit test\n");
    printf("================================================\n\n");
    test_init_and_legality(); printf("\n");
    test_win_four_directions(); printf("\n");
    test_undo_and_draw(); printf("\n");
    test_record_roundtrip(); printf("\n");
    test_record_invalid(); printf("\n");
    test_record_style(); printf("\n");
    printf("================================================\n");
    printf("  Total %d, passed %d, failed %d\n", g_total, g_total - g_failed, g_failed);
    printf("================================================\n");
    return (g_failed == 0) ? 0 : 1;
}
