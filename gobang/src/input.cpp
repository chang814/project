/*==============================================================================
 * 文件名称：input.cpp
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：交互模块              负责人：雷子豪
 * 功能描述：实现鼠标消息的接收与坐标换算。
 *============================================================================*/
#include <graphics.h>

#include "input.h"

/* 棋盘可点击范围：X / Y 两轴各自以对应原点外扩半格。
 * 旧实现横竖都套用 UI_ORIGIN_X，导致底部两行（row 13/14）被判定为越界，
 * 点击时落不到棋盘上——配合 ui.cpp 的边框错位，就表现为“棋子不在中心线”。 */
#define BOARD_MIN_X  (UI_ORIGIN_X - UI_CELL / 2)
#define BOARD_MAX_X  (UI_ORIGIN_X + (BOARD_SIZE - 1) * UI_CELL + UI_CELL / 2)
#define BOARD_MIN_Y  (UI_ORIGIN_Y - UI_CELL / 2)
#define BOARD_MAX_Y  (UI_ORIGIN_Y + (BOARD_SIZE - 1) * UI_CELL + UI_CELL / 2)

int input_pixel_to_cell(int x, int y, int *row, int *col)
{
    int r;
    int c;

    if (row == NULL || col == NULL) return 0;

    *row = -1;
    *col = -1;

    if (x < BOARD_MIN_X || x > BOARD_MAX_X ||
        y < BOARD_MIN_Y || y > BOARD_MAX_Y)
    {
        return 0;
    }

    c = (x - UI_ORIGIN_X + UI_CELL / 2) / UI_CELL;
    r = (y - UI_ORIGIN_Y + UI_CELL / 2) / UI_CELL;

    if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE) return 0;

    *row = r;
    *col = c;
    return 1;
}

int input_hit_button(const UiButton *buttons, int count, int x, int y)
{
    int i;

    if (buttons == NULL || count <= 0) return BTN_NONE;

    for (i = 0; i < count; i++)
    {
        if (x >= buttons[i].x && x <= buttons[i].x + buttons[i].w &&
            y >= buttons[i].y && y <= buttons[i].y + buttons[i].h)
        {
            return buttons[i].id;
        }
    }

    return BTN_NONE;
}

int input_poll(const UiButton *buttons, int count, int allowBoard, UiEvent *ev)
{
    mouse_msg msg;
    int row;
    int col;
    int buttonId;

    if (ev == NULL) return 0;
    if (!mousemsg()) return 0;

    msg = getmouse();

    ev->type     = EVENT_HOVER;
    ev->row      = -1;
    ev->col      = -1;
    ev->buttonId = BTN_NONE;
    ev->mouseX   = msg.x;
    ev->mouseY   = msg.y;

    if (input_pixel_to_cell(msg.x, msg.y, &row, &col))
    {
        ev->hoverRow = row;
        ev->hoverCol = col;
    }
    else
    {
        ev->hoverRow = -1;
        ev->hoverCol = -1;
    }

    buttonId = input_hit_button(buttons, count, msg.x, msg.y);
    if (buttonId != BTN_NONE) ev->buttonId = buttonId;

    if (!(msg.is_left() && msg.is_down())) return 1;

    if (buttonId != BTN_NONE)
    {
        ev->type = EVENT_BUTTON;
        return 1;
    }

    if (ev->hoverRow >= 0 && ev->hoverCol >= 0)
    {
        if (allowBoard)
        {
            ev->type = EVENT_PLACE;
            ev->row  = ev->hoverRow;
            ev->col  = ev->hoverCol;
        }
        else
        {
            ev->type = EVENT_OUTSIDE;
        }
        return 1;
    }

    ev->type = EVENT_OUTSIDE;
    return 1;
}

void input_flush(void)
{
    flushmouse();
}
