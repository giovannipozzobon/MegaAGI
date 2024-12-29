#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>

#include "gfx.h"
#include "pic.h"
#include "sound.h"
#include "view.h"
#include "volume.h"
#include "simplefile.h"
#include "memmanage.h"

#define ASCIIKEY (*(volatile uint8_t *)0xd610)
#define PETSCIIKEY (*(volatile uint8_t *)0xd619)
#define VICREGS ((volatile uint8_t *)0xd000)
#define POKE(X, Y) (*(volatile uint8_t*)(X)) = Y
#define PEEK(X) (*(volatile uint8_t*)(X))

extern uint8_t view_w;
extern uint8_t view_h;

static uint8_t view_sprite = 0;
volatile uint8_t view_x = 40;
volatile uint8_t view_y = 40;
volatile int8_t view_dx = 0;
volatile int8_t view_dy = 0;
volatile uint16_t loop_offset;
volatile uint8_t loop_count;
volatile uint8_t cel_count;
volatile uint8_t cel_index;
volatile uint8_t frame_counter;
static uint16_t view_offset;
static uint8_t picture = 1;
static uint8_t keypress_up;
static uint8_t keypress_down;
static uint8_t keypress_left;
static uint8_t keypress_right;
static uint8_t last_cursorreg;
static uint8_t last_columnzero;
static uint8_t keypress_f5;
static uint8_t object_dir;
static uint8_t loop_index;
volatile uint8_t drawing_screen = 2;
volatile uint8_t viewing_screen = 0;
volatile uint8_t frame_dirty;

void autoselect_loop(void) {
    if (loop_count == 1) {
        return;
    }
    if (loop_count < 4) {
        switch(object_dir) {
            case 2:
            loop_index = 0;
            loop_offset = select_loop(view_offset, 0);
            break;
            case 3:
            loop_index = 0;
            loop_offset = select_loop(view_offset, 0);
            break;
            case 4:
            loop_index = 0;
            loop_offset = select_loop(view_offset, 0);
            break;
            case 6:
            loop_index = 1;
            loop_offset = select_loop(view_offset, 1);
            break;
            case 7:
            loop_index = 1;
            loop_offset = select_loop(view_offset, 1);
            break;
            case 8:
            loop_index = 1;
            loop_offset = select_loop(view_offset, 1);
            break;
        }
    } else {
        switch(object_dir) {
            case 1:
            loop_index = 3;
            loop_offset = select_loop(view_offset, 3);
            break;
            case 2:
            loop_index = 0;
            loop_offset = select_loop(view_offset, 0);
            break;
            case 3:
            loop_index = 0;
            loop_offset = select_loop(view_offset, 0);
            break;
            case 4:
            loop_index = 0;
            loop_offset = select_loop(view_offset, 0);
            break;
            case 5:
            loop_index = 2;
            loop_offset = select_loop(view_offset, 2);
            break;
            case 6:
            loop_index = 1;
            loop_offset = select_loop(view_offset, 1);
            break;
            case 7:
            loop_index = 1;
            loop_offset = select_loop(view_offset, 1);
            break;
            case 8:
            loop_index = 1;
            loop_offset = select_loop(view_offset, 1);
            break;
        }
    }
}

/*
    0000 = inv
    0001 = inv
    0010 = inv
    0011 = inv
    0100 = inv
    0101 = right/down
    0110 = right/up
    0111 = right
    1000 = inv
    1001 = left/down
    1010 = left/up
    1011 = left
    1100 = inv
    1101 = down
    1110 = up
    1111 = inv
*/

void set_object_motions(void) {
    __disable_interrupts();
    switch (object_dir) {
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
    __enable_interrupts();
}

void handle_movement_joystick(void) {
    const uint8_t joystick_direction[16] = {
        0, 0, 0, 0, 0, 4, 2, 3,
        0, 6, 8, 7, 0, 5, 1, 0
    };
    uint8_t port2joy = PEEK(0xDC00) & 0x0f;
    uint8_t new_object_dir = joystick_direction[port2joy];
    if (new_object_dir != object_dir) {
        object_dir = new_object_dir;
        set_object_motions();
        autoselect_loop();
    }
}

/*
void handle_movement_keys(void) {
    __disable_interrupts();
    POKE(0xd614, 0);
    uint8_t cursor_reg = PEEK(0xd60f);
    uint8_t column_zero = PEEK(0xd613);
    if (cursor_reg != last_cursorreg) {
        column_zero = 0xff;
    }
    last_cursorreg = cursor_reg;
    if (column_zero != last_columnzero) {
        last_columnzero = column_zero;
        column_zero = 0xff;
    }
    __enable_interrupts();
    uint8_t up_key = cursor_reg & 0x02;
    uint8_t left_key = cursor_reg & 0x01;
    uint8_t right_key = (!(column_zero & 0x04)) && (!left_key);
    uint8_t down_key = (!(column_zero & 0x80)) && (!up_key);
    keypress_f5 = (!(column_zero & 0x40));
    if (down_key) {
        if (!keypress_down) {
            keypress_down = 1;
            object_dir = 5;
            set_dy(1);
            autoselect_loop();
        } 
    } else {
        keypress_down = 0;
    }
    if (right_key) {
        if (!keypress_right) {
            keypress_right = 1;
            object_dir = 3;
            set_dx(1);
            autoselect_loop();
        }
    } else {
        keypress_right = 0;
    }
    if (up_key) {
        if (!keypress_up) {
            keypress_up = 1;
            object_dir = 1;
            set_dy(-1);
            autoselect_loop();
        }
    } else {
        keypress_up = 0;
    }
    if (left_key) {
        if (!keypress_left) {
            keypress_left = 1;
            object_dir = 7;
            set_dx(-1);
            autoselect_loop();
        }
    } else {
        keypress_left = 0;
    }
}*/

void draw_sprite(void) {
    erase_view();
    cel_index++;
    if (cel_index == cel_count) {
        cel_index = 0;
    }
    draw_cel(loop_offset, loop_index, cel_index, view_x, view_y);
}

void parse_debug_command(char *command) {
    char *cmd = strtok(command, " ");
    if (0 == strcmp(cmd, "PIC")) {
        char *arg = strtok(NULL, " ");
        if (arg == NULL) {
            return;
        }
        view_dx = 0;
        view_dy = 0;
        uint8_t pic_num = atoi(arg);
        draw_pic(drawing_screen, pic_num, 0);
        draw_cel(loop_offset, loop_index, 0, view_x, view_y);
        drawing_screen ^= viewing_screen;
        viewing_screen ^= drawing_screen;
        drawing_screen ^= viewing_screen;
        gfx_showgfx(viewing_screen);
        gfx_copygfx(viewing_screen);
    } else if (0 == strcmp(cmd, "SOUND")) {
        char *arg = strtok(NULL, " ");
        if (arg == NULL) {
            return;
        }
        uint8_t sound_num = atoi(arg);
        play_sound(sound_num);
    } else if (0 == strcmp(cmd, "VIEW")) {
        char *arg = strtok(NULL, " ");
        if (arg == NULL) {
            return;
        }
        erase_view();
        view_sprite = atoi(arg);
        view_dx = 0;
        view_dy = 0;
        view_x = 40;
        view_y = 40;
        chipmem_free(view_offset);
        view_offset = load_view(view_sprite);
        loop_count = get_num_loops(view_offset);
        object_dir = 0;
        loop_offset = select_loop(view_offset, 0);
        cel_count = get_num_cels(loop_offset);
        cel_index = 0;
        loop_index = 0;
        draw_sprite();
        frame_dirty = 1;
    }
}

void run_loop(void) {
    static char command_buffer[38];
    static uint8_t cmd_buf_ptr = 0;

    gfx_switchto();
    draw_pic(0, picture, 0);
    gfx_copygfx(0);
    view_offset = load_view(view_sprite);
    loop_count = get_num_loops(view_offset);
    object_dir = 0;
    loop_offset = select_loop(view_offset, 0);
    cel_count = get_num_cels(loop_offset);
    cel_index = 0;
    loop_index = 0;
    gfx_print_parserline('\r');
    gfx_showgfx(0);
    hook_irq();
    draw_sprite();
    frame_dirty = 0;
    while (1) {
        handle_movement_joystick();
        if (frame_counter == 0) {
            if (ASCIIKEY != 0) {
                uint8_t petscii = PETSCIIKEY;
                ASCIIKEY = 0;
                if (petscii == 0x14) {
                    if (cmd_buf_ptr > 0) {
                        cmd_buf_ptr--;
                        gfx_print_parserline(petscii);
                    }
                } else if (petscii == 0x0d) {
                    command_buffer[cmd_buf_ptr] = 0;
                    parse_debug_command(command_buffer);
                    cmd_buf_ptr=0;
                    gfx_print_parserline(petscii);
                } else {
                    if (cmd_buf_ptr < 37) {
                        command_buffer[cmd_buf_ptr] = petscii;
                        gfx_print_parserline(petscii);
                        cmd_buf_ptr++;
                    }
                }
            }

            view_x += view_dx;
            view_y += view_dy;
            if (view_x == 255) {
                view_x = 0;
                view_dx = 0;
            } else if (view_y == 255) {
                view_y = 0;
                view_dy = 0;
            } else if (view_x > (160 - view_w)) {
                view_x = 160 - view_w;
                view_dx = 0;
            } else if (view_y > (167 - view_h)) {
                view_y = 167 - view_h;
                view_dy = 0;
            }
            if ((view_dx != 0) || (view_dy != 0)) {
                draw_sprite();
                frame_dirty = 1;
            }
            while(frame_dirty);
        }
    }
}

void engine_interrupt_handler(void) {
    frame_counter++;
    if (frame_counter < 2) {
        return;
    }
    frame_counter = 0;
    if (frame_dirty) {
        drawing_screen ^= viewing_screen;
        viewing_screen ^= drawing_screen;
        drawing_screen ^= viewing_screen;
        gfx_showgfx(viewing_screen);
        gfx_copygfx(viewing_screen);
        frame_dirty = 0;
    }
}
