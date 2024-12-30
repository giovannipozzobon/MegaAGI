#ifndef SPRITE_H
#define SPRITE_H

void autoselect_loop(uint8_t sprite_num);
uint8_t sprite_move(uint8_t sprite_num);
void sprite_set_direction(uint8_t sprite_num, uint8_t direction);
void sprite_set_position(uint8_t sprite_num, uint8_t pos_x, uint8_t pos_y);
void sprite_erase(uint8_t sprite_num);
void sprite_draw(uint8_t sprite_num);
void sprite_set_view(uint8_t sprite_num, uint8_t view_number);

#endif