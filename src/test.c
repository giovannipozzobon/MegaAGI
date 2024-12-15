#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <mega65.h>

void sound_test(void) {
    SID1.amp = 0x0f;
    SID1.v1.ad = 0x00;
    SID1.v1.sr = 0xf0;
    SID1.v1.freq = 0x1150;
    SID1.v1.ctrl = 0x21;
    SID1.v2.ad = 0x00;
    SID1.v2.sr = 0xf0;
    SID1.v2.freq = 0x15D0;
    SID1.v2.ctrl = 0x21;
    SID1.v3.ad = 0x00;
    SID1.v3.sr = 0xf0;
    SID1.v3.freq = 0x19F0;
    SID1.v3.ctrl = 0x21;
    while(1);
}