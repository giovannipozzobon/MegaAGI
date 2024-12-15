#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>
#include <math.h>

#include "gfx.h"
#include "simplefile.h"
#include "memmanage.h"
#include "volume.h"
#include "view.h"

uint8_t view_backbuf[1024];
int16_t pos_x;
int16_t pos_y;
volatile uint8_t view_w;
volatile uint8_t view_h;
#define ASCIIKEY (*(volatile uint8_t *)0xd610)
volatile uint8_t was_mirrored;

uint8_t get_priority(uint8_t y) {
    if (y == 168) {
        return 15;
    }
    if (y > 156) {
        return 14;
    }
    if (y > 144) {
        return 13;
    }
    if (y > 132) {
        return 12;
    }
    if (y > 120) {
        return 11;
    }
    if (y > 108) {
        return 10;
    }
    if (y > 96) {
        return 9;
    }
    if (y > 84) {
        return 8;
    }
    if (y > 72) {
        return 7;
    }
    if (y > 60) {
        return 6;
    }
    if (y > 48) {
        return 5;
    }
    return 4;
}

void draw_cel_forward(uint8_t __far *cel_data, int16_t x, int16_t y) {
    uint8_t objprio = get_priority(y + view_h);
    uint8_t right_side = x + view_w;
    uint8_t view_trans = cel_data[2] & 0x0f;
    int16_t cur_x = x;
    int16_t cur_y = y;
    uint16_t cel_offset = 3;
    uint16_t pixel_offset = 0;
    uint8_t draw_row = 0;

    do {
        uint8_t pixcol = cel_data[cel_offset] >> 4;
        uint8_t pixcnt = cel_data[cel_offset] & 0x0f;
        if ((pixcol == 0) && (pixcnt == 0)) {
            for (; cur_x < right_side; cur_x++) {
                view_backbuf[pixel_offset] = gfx_get(0, cur_x, cur_y);
                pixel_offset++;
            }
            cur_x = x;
            cur_y++;
            draw_row++;
        } else {
            for (uint8_t count = 0; count < pixcnt; count++) {
                view_backbuf[pixel_offset] = gfx_get(0, cur_x, cur_y);
                pixel_offset++;
                if (pixcol != view_trans) {
                    uint8_t pix_prio = gfx_getprio(cur_x, cur_y);
                    if (objprio >= pix_prio) {
                        gfx_plotput(0, cur_x, cur_y, pixcol);
                    }
                }
                cur_x++;
            }
        }
        cel_offset++;
    } while (draw_row < view_h);
}

void draw_cel_backward(uint8_t __far *cel_data, int16_t x, int16_t y) {
    uint8_t objprio = get_priority(y + view_h);
    uint8_t right_side = x + view_w;
    uint8_t view_trans = cel_data[2] & 0x0f;
    int16_t cur_x = right_side;
    int16_t cur_y = y;
    uint16_t cel_offset = 3;
    uint16_t pixel_offset = 0;
    uint8_t draw_row = 0;

    do {
        uint8_t pixcol = cel_data[cel_offset] >> 4;
        uint8_t pixcnt = cel_data[cel_offset] & 0x0f;
        if ((pixcol == 0) && (pixcnt == 0)) {
            for (; cur_x >= x; cur_x--) {
                view_backbuf[pixel_offset] = gfx_get(0, cur_x, cur_y);
                pixel_offset++;
            }
            cur_x = right_side;
            cur_y++;
            draw_row++;
        } else {
            for (uint8_t count = 0; count < pixcnt; count++) {
                view_backbuf[pixel_offset] = gfx_get(0, cur_x, cur_y);
                pixel_offset++;
                if (pixcol != view_trans) {
                    uint8_t pix_prio = gfx_getprio(cur_x, cur_y);
                    if (objprio >= pix_prio) {
                        gfx_plotput(0, cur_x, cur_y, pixcol);
                    }
                }
                cur_x--;
            }
        }
        cel_offset++;
    } while (draw_row < view_h);
}

void draw_cel(uint16_t loop_offset, uint8_t loop_index, uint8_t cel, uint8_t x, uint8_t y) {
    pos_x = x;
    pos_y = y;

    uint8_t __far *loop_data = chipmem_base + loop_offset;
    uint8_t __far *cell_ptr = loop_data + (cel * 2) + 1;
    uint16_t cel_base = *cell_ptr | ((*(cell_ptr + 1)) << 8);
    uint8_t __far *cel_data = loop_data + cel_base;

    view_w = cel_data[0];
    view_h = cel_data[1];
    uint8_t mirrored = cel_data[2] & 0x80;
    uint8_t source_loop = (cel_data[2] & 0x70) >> 4;
    if (!mirrored || (mirrored && (source_loop == loop_index))) {
        draw_cel_forward(cel_data, x, y);
        was_mirrored = 0;
    } else {
        draw_cel_backward(cel_data, x, y);
        was_mirrored = 1;
    }
}

void erase_view(void) {
    uint16_t pixel_offset = 0;
    uint8_t right_side = pos_x + view_w;
    uint8_t bottom_side = pos_y + view_h;
    if (was_mirrored) {
        for (int16_t cur_y = pos_y; cur_y < bottom_side; cur_y++) {
            for (int16_t cur_x = right_side; cur_x >= pos_x; cur_x--) {
                gfx_plotput(0, cur_x, cur_y, view_backbuf[pixel_offset]);
                pixel_offset++;
            }
        }
    } else {
        for (uint8_t cur_y = pos_y; cur_y < bottom_side; cur_y++) {
            for (uint8_t cur_x = pos_x; cur_x < right_side; cur_x++) {
                gfx_plotput(0, cur_x, cur_y, view_backbuf[pixel_offset]);
                pixel_offset++;
            }
        }
    }
}

uint8_t get_num_cels(uint16_t loop_offset) {
    uint8_t __far *loop_data = chipmem_base + loop_offset;
    return loop_data[0];
}

uint16_t select_loop(uint16_t view_offset, uint8_t loop_num) {
    uint8_t __far *view_data = chipmem_base + view_offset;
    uint8_t num_loops = view_data[2];
    if (loop_num < num_loops) {
        uint8_t __far *loop_data = view_data + (loop_num * 2) + 5;
        uint16_t loop_offset = *loop_data | ((*(loop_data + 1)) << 8);
        return loop_offset + view_offset;
    }
    else {
        return 0;
    }
}

uint8_t get_num_loops(uint16_t view_offset) {
    uint8_t __far *view_data = chipmem_base + view_offset;
    return view_data[2];
}

uint16_t load_view(uint8_t view_num) {

    uint8_t __huge *view_file;
    uint16_t length;
    view_file = locate_volume_object(voView, view_num, &length);
    if (view_file == NULL) {
        return 0;
    }
    uint16_t view_offset = chipmem_alloc(length);
    uint8_t __far *view_target = chipmem_base + view_offset;
    for (uint16_t counter = 0; counter < length; counter++) {
        view_target[counter] = view_file[counter];
    }
    return view_offset;
}
