#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <mega65.h>

#include "sound.h"
#include "irq.h"
#include "simplefile.h"
#include "volume.h"

#define ASCIIKEY (*(volatile uint8_t *)0xd610)

uint8_t __huge *sound_file;
volatile uint16_t voice_offsets[3];
volatile uint8_t voice_stopped[3] = {0};
volatile uint16_t durations[3] = {0};
volatile uint8_t voice_holds[3];

void play_sound(uint8_t sound_num) {
    uint16_t length;
    sound_file = locate_volume_object(voSound, sound_num, &length);
    if (sound_file == NULL) {
        return;
    }
    voice_offsets[0] = (sound_file[1] << 8) | sound_file[0];
    voice_offsets[1] = (sound_file[3] << 8) | sound_file[2];
    voice_offsets[2] = (sound_file[5] << 8) | sound_file[4];

    SID1.amp = 0x0f;
    SID1.v1.ad = 0x11;
    SID1.v1.sr = 0xf0;
    SID1.v2.pw = 0x400;
    SID1.v2.ad = 0x11;
    SID1.v2.sr = 0xc0;
    SID1.v2.pw = 0x200;
    SID1.v3.ad = 0x11;
    SID1.v3.sr = 0xc0;
    SID1.v2.pw = 0x200;

    durations[0] = 1;
    durations[1] = 1;
    durations[2] = 1;
    voice_stopped[0] = 0;
    voice_stopped[1] = 0;
    voice_stopped[2] = 0;
}

void wait_sound(void) {
    while (!voice_stopped[0] || !voice_stopped[1] || !voice_stopped[2]) {
    }
}

void sound_interrupt_handler(void) {
    for (uint8_t voice=0; voice < 3; voice++) {
        --durations[voice];
        if (!voice_stopped[voice]) {
            uint16_t voice_offset = voice_offsets[voice];
            volatile struct __sid_voice *sid;
            switch(voice) {
                case 0:
                    sid = &SID1.v1;
                    break;
                case 1:
                    sid = &SID1.v2;
                    break;
                case 2:
                    sid = &SID1.v3;
                    break;
            }
            if (durations[voice] < voice_holds[voice]) {
                sid->ctrl = 0x00;
            }
            if (durations[voice] == 0) {
                uint16_t duration = (sound_file[voice_offset+1] << 8);
                duration = duration + sound_file[voice_offset];
                if (duration == 0xffff) {
                    sid->ctrl = 0x00;
                    voice_stopped[voice] = 1;
                    continue;
                }
                uint16_t fnum = (((sound_file[voice_offset + 2] & 0x3f) << 4) + (sound_file[voice_offset + 3] & 0x0f));
                uint16_t vol = sound_file[voice_offset + 4];
                voice_offsets[voice] = voice_offsets[voice] + 5;
                if ((vol & 0x0f) == 0x0f) {
                    sid->ctrl = 0x00;
                } else {
                    uint16_t sidctrl = 1876426 / (fnum - 1);
                    sid->freq = sidctrl;
                    sid->ctrl = 0x41;
                }
                durations[voice] = duration;
                if (duration < 6) {
                    voice_holds[voice] = 0;
                } else {
                    voice_holds[voice] = 3;
                }
            }
        }
    }
}