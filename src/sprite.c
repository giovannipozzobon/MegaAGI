#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>

#include "memmanage.h"
#include "view.h"

extern uint8_t view_w;
extern uint8_t view_h;

typedef struct {
    int16_t pos_x;
    int16_t pos_y;
    int16_t oldpos_x;
    int16_t oldpos_y;

    uint8_t view_w;
    uint8_t view_h;

    uint16_t view_offset;

    uint16_t loop_offset;
    uint8_t loop_count;
    uint8_t loop_index;
    uint8_t cel_count;
    uint8_t cel_index;

    uint8_t object_dir;

} agisprite_t;

#pragma clang section bss="extradata"
__far static agisprite_t sprites[256];
#pragma clang section bss=""

void autoselect_loop(uint8_t sprite_num) {
    agisprite_t sprite = sprites[sprite_num];
    if (sprite.loop_count == 1) {
        return;
    }
    if (sprite.loop_count < 4) {
        switch(sprite.object_dir) {
            case 2:
            sprite.loop_index = 0;
            sprite.loop_offset = select_loop(sprite.view_offset, 0);
            break;
            case 3:
            sprite.loop_index = 0;
            sprite.loop_offset = select_loop(sprite.view_offset, 0);
            break;
            case 4:
            sprite.loop_index = 0;
            sprite.loop_offset = select_loop(sprite.view_offset, 0);
            break;
            case 6:
            sprite.loop_index = 1;
            sprite.loop_offset = select_loop(sprite.view_offset, 1);
            break;
            case 7:
            sprite.loop_index = 1;
            sprite.loop_offset = select_loop(sprite.view_offset, 1);
            break;
            case 8:
            sprite.loop_index = 1;
            sprite.loop_offset = select_loop(sprite.view_offset, 1);
            break;
        }
    } else {
        switch(sprite.object_dir) {
            case 1:
            sprite.loop_index = 3;
            sprite.loop_offset = select_loop(sprite.view_offset, 3);
            break;
            case 2:
            sprite.loop_index = 0;
            sprite.loop_offset = select_loop(sprite.view_offset, 0);
            break;
            case 3:
            sprite.loop_index = 0;
            sprite.loop_offset = select_loop(sprite.view_offset, 0);
            break;
            case 4:
            sprite.loop_index = 0;
            sprite.loop_offset = select_loop(sprite.view_offset, 0);
            break;
            case 5:
            sprite.loop_index = 2;
            sprite.loop_offset = select_loop(sprite.view_offset, 2);
            break;
            case 6:
            sprite.loop_index = 1;
            sprite.loop_offset = select_loop(sprite.view_offset, 1);
            break;
            case 7:
            sprite.loop_index = 1;
            sprite.loop_offset = select_loop(sprite.view_offset, 1);
            break;
            case 8:
            sprite.loop_index = 1;
            sprite.loop_offset = select_loop(sprite.view_offset, 1);
            break;
        }
    }
    sprites[sprite_num] = sprite;
}

uint8_t sprite_move(uint8_t sprite_num) {
    int16_t view_dx;
    int16_t view_dy;
    agisprite_t sprite = sprites[sprite_num];
    switch (sprite.object_dir) {
        case 0:
        view_dx = 0;
        view_dy = 0;
        break;
        case 1:
        view_dx = 0;
        view_dy = -1;
        break;
        case 2:
        view_dx = 1;
        view_dy = -1;
        break;
        case 3:
        view_dx = 1;
        view_dy = 0;
        break;
        case 4:
        view_dx = 1;
        view_dy = 1;
        break;
        case 5:
        view_dx = 0;
        view_dy = 1;
        break;
        case 6:
        view_dx = -1;
        view_dy = 1;
        break;
        case 7:
        view_dx = -1;
        view_dy = 0;
        break;
        case 8:
        view_dx = -1;
        view_dy = -1;
        break;
    }

    sprite.pos_x += view_dx;
    sprite.pos_y += view_dy;
    if (sprite.pos_x == -1) {
        sprite.pos_x = 0;
        view_dx = 0;
    } else if (sprite.pos_x > (159 - sprite.view_w)) {
        sprite.pos_x = 159 - sprite.view_w;
        view_dx = 0;
    }
    
    if (sprite.pos_y == -1) {
        sprite.pos_y = 0;
        view_dy = 0;
    } else if (sprite.pos_y > (167 - sprite.view_h)) {
        sprite.pos_y = 167 - sprite.view_h;
        view_dy = 0;
    }

    sprites[sprite_num] = sprite;
    return ((view_dx != 0) || (view_dy != 0));
}

void sprite_set_direction(uint8_t sprite_num, uint8_t direction) {
    sprites[sprite_num].object_dir = direction;
    autoselect_loop(sprite_num);
}

void sprite_set_position(uint8_t sprite_num, uint8_t pos_x, uint8_t pos_y) {
    sprites[sprite_num].oldpos_x = sprites[sprite_num].pos_x = pos_x;
    sprites[sprite_num].oldpos_y = sprites[sprite_num].pos_y = pos_y;
}

void sprite_erase(uint8_t sprite_num) {
    agisprite_t sprite = sprites[sprite_num];
    erase_view(sprite.pos_x, sprite.pos_y);
}

void sprite_draw(uint8_t sprite_num) {
    agisprite_t sprite = sprites[sprite_num];
    erase_view(sprite.oldpos_x, sprite.oldpos_y);
    sprite.cel_index++;
    if (sprite.cel_index == sprite.cel_count) {
        sprite.cel_index = 0;
    }
    draw_cel(sprite.loop_offset, sprite.loop_index, sprite.cel_index, sprite.pos_x, sprite.pos_y);
    sprite.view_w = view_w;
    sprite.view_h = view_h;
    sprite.oldpos_x = sprite.pos_x;
    sprite.oldpos_y = sprite.pos_y;
    sprites[sprite_num] = sprite;
}

void sprite_set_view(uint8_t sprite_num, uint8_t view_number) {
    agisprite_t sprite = sprites[sprite_num];

  //  if (sprite.view_offset != 0) {
  //      chipmem_free(sprite.view_offset);
 //   }

    sprite.view_offset = load_view(view_number);
    sprite.loop_count = get_num_loops(sprite.view_offset);
    sprite.object_dir = 0;
    sprite.loop_offset = select_loop(sprite.view_offset, 0);
    sprite.cel_count = get_num_cels(sprite.loop_offset);
    sprite.cel_index = 0;
    sprite.loop_index = 0;
    sprites[sprite_num] = sprite;
}
