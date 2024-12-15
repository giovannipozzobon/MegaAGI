#ifndef SOUND_H
#define SOUND_H
void play_sound(uint8_t sound_num);
void wait_sound(void);
void sound_interrupt_handler(void);

#endif