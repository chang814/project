/*==============================================================================
 * test_input.cpp - Unit tests for the input module (pure functions, ASCII).
 *============================================================================*/
#include <stdio.h>

#include "input.h"

static int g_total = 0;
static int g_failed = 0;

#define CHECK(cond, name)                                                   \
    do {                                                                    \
        g_total++;                                                          \
        if (cond) { printf("  [PASS] %s\n", name); }                        \
        else { g_failed++;                                                  \
               printf("  [FAIL] %s   (%s:%d)\n", name, __FILE__, __LINE__); } \
    } while (0)

int main(void)
{
    int row, col;

    printf("================================================\n");
    printf("  Input module unit test\n");
    printf("================================================\n\n");

    printf("T1 pixel-to-cell mapping\n");
    CHECK(input_pixel_to_cell(320, 320, &row, &col) == 1 && row == 7 && col == 7, "exact center maps to (7,7)");
    CHECK(input_pixel_to_cell(330, 320, &row, &col) == 1 && row == 7 && col == 7, "offset within half cell snaps to (7,7)");
    CHECK(input_pixel_to_cell(40, 40, &row, &col) == 1 && row == 0 && col == 0, "top-left corner maps to (0,0)");
    CHECK(input_pixel_to_cell(600, 600, &row, &col) == 1 && row == 14 && col == 14, "bottom-right corner maps to (14,14)");

    printf("T2 out-of-board rejection\n");
    CHECK(input_pixel_to_cell(5, 320, &row, &col) == 0, "left margin rejected");
    CHECK(input_pixel_to_cell(700, 320, &row, &col) == 0, "right panel rejected");
    CHECK(input_pixel_to_cell(320, 5, &row, &col) == 0, "top margin rejected");
    CHECK(input_pixel_to_cell(900, 600, &row, &col) == 0, "window corner rejected");

    printf("T3 button hit test\n");
    {
        UiButton buttons[2];
        buttons[0].x = 100; buttons[0].y = 100; buttons[0].w = 200; buttons[0].h = 50; buttons[0].id = 1; buttons[0].text = "A";
        buttons[1].x = 400; buttons[1].y = 400; buttons[1].w = 100; buttons[1].h = 40; buttons[1].id = 2; buttons[1].text = "B";
        CHECK(input_hit_button(buttons, 2, 200, 125) == 1, "click inside button 1 hits it");
        CHECK(input_hit_button(buttons, 2, 450, 420) == 2, "click inside button 2 hits it");
        CHECK(input_hit_button(buttons, 2, 350, 300) == BTN_NONE, "click in empty area hits nothing");
        CHECK(input_hit_button(buttons, 2, 100, 100) == 1, "top-left edge of button 1 hits");
        CHECK(input_hit_button(buttons, 2, 99, 100) == BTN_NONE, "one pixel left of button 1 misses");
    }

    printf("\n================================================\n");
    printf("  Total %d, passed %d, failed %d\n", g_total, g_total - g_failed, g_failed);
    printf("================================================\n");
    return (g_failed == 0) ? 0 : 1;
}
