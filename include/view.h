#ifndef VIEW_H
#define VIEW_H

void draw_cel(uint16_t loop_offset, uint8_t view_index, uint8_t cel, uint8_t x, uint8_t y);
void erase_view(int16_t x, int16_t y);
uint8_t get_num_cels(uint16_t loop_offset);
uint16_t select_loop(uint16_t view_offset, uint8_t loop_num);
uint8_t get_num_loops(uint16_t view_offset);
uint16_t load_view(uint8_t view_num);

#endif