/*==============================================================================
 * 文件名称：input.h
 * 所属项目：五子棋对战游戏（双人·人机·联网对战）  第15小组
 * 模块归属：交互模块              负责人：雷子豪
 * 功能描述：声明鼠标消息的接收与转换接口。
 *============================================================================*/
#ifndef GOBANG_INPUT_H
#define GOBANG_INPUT_H

#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_NONE     0
#define EVENT_PLACE    1
#define EVENT_BUTTON   2
#define EVENT_HOVER    3
#define EVENT_OUTSIDE  4

typedef struct
{
    int type;
    int row;
    int col;
    int buttonId;
    int mouseX;
    int mouseY;
    int hoverRow;
    int hoverCol;
} UiEvent;

int input_pixel_to_cell(int x, int y, int *row, int *col);
int input_hit_button(const UiButton *buttons, int count, int x, int y);
int input_poll(const UiButton *buttons, int count, int allowBoard, UiEvent *ev);
void input_flush(void);

#ifdef __cplusplus
}
#endif

#endif /* GOBANG_INPUT_H */
