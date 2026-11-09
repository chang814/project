/*==============================================================================
 * 文件名称：board.c
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：棋局逻辑模块          负责人：chang814
 * 功能描述：实现棋盘初始化、落子合法性检查、落子与悔棋、四方向胜负判定、和棋判定。
 *============================================================================*/
#include "board.h"

#include <string.h>

static const int DIR_STEP[DIR_COUNT][2] =
{
    { 0,  1 },
    { 1,  0 },
    { 1,  1 },
    { 1, -1 }
};

void board_init(Board *board)
{
    int row;
    int col;

    if (board == NULL)
    {
        return;
    }

    for (row = 0; row < BOARD_SIZE; row++)
    {
        for (col = 0; col < BOARD_SIZE; col++)
        {
            board->grid[row][col] = CELL_EMPTY;
        }
    }

    board->moveCount  = 0;
    board->current    = CELL_BLACK;
    board->status     = GAME_PLAYING;
    board->hasWinLine = 0;

    memset(board->winLine, 0, sizeof(board->winLine));
    memset(board->moves,   0, sizeof(board->moves));
}

int board_in_range(int row, int col)
{
    return (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE);
}

int board_get(const Board *board, int row, int col)
{
    if (board == NULL || !board_in_range(row, col))
    {
        return CELL_EMPTY;
    }
    return board->grid[row][col];
}

int board_can_place(const Board *board, int row, int col)
{
    if (board == NULL)
    {
        return PLACE_ERR_NULL;
    }
    if (board->status != GAME_PLAYING)
    {
        return PLACE_ERR_FINISHED;
    }
    if (!board_in_range(row, col))
    {
        return PLACE_ERR_RANGE;
    }
    if (board->grid[row][col] != CELL_EMPTY)
    {
        return PLACE_ERR_OCCUPIED;
    }
    return PLACE_OK;
}

int board_count_line(const Board *board, int row, int col, int dirIndex, int player)
{
    int deltaRow;
    int deltaCol;
    int count;
    int step;
    int r;
    int c;

    if (board == NULL || !board_in_range(row, col))
    {
        return 0;
    }
    if (dirIndex < 0 || dirIndex >= DIR_COUNT)
    {
        return 0;
    }

    deltaRow = DIR_STEP[dirIndex][0];
    deltaCol = DIR_STEP[dirIndex][1];

    count = 1;

    for (step = 1; step < BOARD_SIZE; step++)
    {
        r = row + deltaRow * step;
        c = col + deltaCol * step;
        if (!board_in_range(r, c) || board->grid[r][c] != player)
        {
            break;
        }
        count++;
    }

    for (step = 1; step < BOARD_SIZE; step++)
    {
        r = row - deltaRow * step;
        c = col - deltaCol * step;
        if (!board_in_range(r, c) || board->grid[r][c] != player)
        {
            break;
        }
        count++;
    }

    return count;
}

static void fill_win_line(const Board *board, int row, int col, int dirIndex,
                          int player, int line[WIN_COUNT][2])
{
    int deltaRow = DIR_STEP[dirIndex][0];
    int deltaCol = DIR_STEP[dirIndex][1];
    int r = row;
    int c = col;
    int i;

    while (board_in_range(r - deltaRow, c - deltaCol) &&
           board->grid[r - deltaRow][c - deltaCol] == player)
    {
        r -= deltaRow;
        c -= deltaCol;
    }

    for (i = 0; i < WIN_COUNT; i++)
    {
        line[i][0] = r + deltaRow * i;
        line[i][1] = c + deltaCol * i;
    }
}

int board_check_win(const Board *board, int row, int col, int player, int line[WIN_COUNT][2])
{
    int dirIndex;
    int count;

    if (board == NULL || !board_in_range(row, col))
    {
        return 0;
    }
    if (player != CELL_BLACK && player != CELL_WHITE)
    {
        return 0;
    }
    if (board->grid[row][col] != player)
    {
        return 0;
    }

    for (dirIndex = 0; dirIndex < DIR_COUNT; dirIndex++)
    {
        count = board_count_line(board, row, col, dirIndex, player);
        if (count >= WIN_COUNT)
        {
            if (line != NULL)
            {
                fill_win_line(board, row, col, dirIndex, player, line);
            }
            return 1;
        }
    }

    return 0;
}

int board_is_full(const Board *board)
{
    if (board == NULL)
    {
        return 0;
    }
    return (board->moveCount >= MAX_MOVES);
}

int board_place(Board *board, int row, int col)
{
    int code;
    int player;

    code = board_can_place(board, row, col);
    if (code != PLACE_OK)
    {
        return code;
    }

    player = board->current;

    board->grid[row][col] = player;

    board->moves[board->moveCount].row    = row;
    board->moves[board->moveCount].col    = col;
    board->moves[board->moveCount].player = player;
    board->moveCount++;

    if (board_check_win(board, row, col, player, board->winLine))
    {
        board->hasWinLine = 1;
        board->status = (player == CELL_BLACK) ? GAME_BLACK_WIN : GAME_WHITE_WIN;
    }
    else if (board_is_full(board))
    {
        board->status = GAME_DRAW;
    }
    else
    {
        board->current = OPPONENT(player);
    }

    return PLACE_OK;
}

int board_undo(Board *board)
{
    Move *last;

    if (board == NULL || board->moveCount <= 0)
    {
        return 0;
    }

    board->moveCount--;
    last = &board->moves[board->moveCount];

    board->grid[last->row][last->col] = CELL_EMPTY;
    board->current    = last->player;
    board->status     = GAME_PLAYING;
    board->hasWinLine = 0;

    return 1;
}

int board_undo_steps(Board *board, int steps)
{
    int done = 0;

    if (board == NULL || steps <= 0)
    {
        return 0;
    }

    while (done < steps && board_undo(board))
    {
        done++;
    }
    return done;
}

void board_resign(Board *board, int player)
{
    if (board == NULL || board->status != GAME_PLAYING)
    {
        return;
    }
    if (player != CELL_BLACK && player != CELL_WHITE)
    {
        return;
    }

    board->status = (player == CELL_BLACK) ? GAME_WHITE_WIN : GAME_BLACK_WIN;
    board->hasWinLine = 0;
}

void board_set_status(Board *board, int status)
{
    if (board == NULL)
    {
        return;
    }
    if (status < GAME_PLAYING || status > GAME_DRAW)
    {
        return;
    }
    board->status = status;
}

const Move *board_last_move(const Board *board)
{
    if (board == NULL || board->moveCount <= 0)
    {
        return NULL;
    }
    return &board->moves[board->moveCount - 1];
}

const char *board_place_error(int code)
{
    switch (code)
    {
    case PLACE_OK:
        return "落子成功";
    case PLACE_ERR_RANGE:
        return "该位置在棋盘之外，请点击棋盘内的交叉点";
    case PLACE_ERR_OCCUPIED:
        return "该交叉点已有棋子，请另选空点";
    case PLACE_ERR_FINISHED:
        return "本局已结束，请重新开始";
    case PLACE_ERR_NULL:
        return "棋局数据无效";
    default:
        return "未知错误";
    }
}
