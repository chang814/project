/*==============================================================================
 * 文件名称：board.h
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：棋局逻辑模块          负责人：chang814
 * 功能描述：声明棋局数据结构 Board 与全部对外接口。
 *============================================================================*/
#ifndef GOBANG_BOARD_H
#define GOBANG_BOARD_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PLACE_OK            0
#define PLACE_ERR_RANGE   (-1)
#define PLACE_ERR_OCCUPIED (-2)
#define PLACE_ERR_FINISHED (-3)
#define PLACE_ERR_NULL    (-4)

#define DIR_HORIZONTAL  0
#define DIR_VERTICAL    1
#define DIR_DIAGONAL    2
#define DIR_ANTIDIAG    3
#define DIR_COUNT       4

typedef struct
{
    int row;
    int col;
    int player;
} Move;

typedef struct
{
    int  grid[BOARD_SIZE][BOARD_SIZE];
    Move moves[MAX_MOVES];
    int  moveCount;
    int  current;
    int  status;
    int  winLine[WIN_COUNT][2];
    int  hasWinLine;
} Board;

void board_init(Board *board);
int board_in_range(int row, int col);
int board_get(const Board *board, int row, int col);
int board_can_place(const Board *board, int row, int col);
int board_place(Board *board, int row, int col);
int board_undo(Board *board);
int board_undo_steps(Board *board, int steps);
int board_is_full(const Board *board);
int board_count_line(const Board *board, int row, int col, int dirIndex, int player);
int board_check_win(const Board *board, int row, int col, int player, int line[WIN_COUNT][2]);
void board_resign(Board *board, int player);
void board_set_status(Board *board, int status);
const Move *board_last_move(const Board *board);
const char *board_place_error(int code);

#ifdef __cplusplus
}
#endif

#endif /* GOBANG_BOARD_H */
