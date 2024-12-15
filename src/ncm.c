#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>

#include "gfx.h"

uint8_t vic4cache[14];
volatile uint8_t __far *graphics_memory[10];
volatile uint8_t __far *prio_memory[10];

struct __F018 {
  uint8_t dmalowtrig;
  uint8_t dmahigh;
  uint8_t dmabank;
  uint8_t enable018;
  uint8_t addrmb;
  uint8_t etrig;
};

/// DMA controller
#define DMA (*(volatile struct __F018 *)0xd700)

#define Q15_16_INT_TO_Q(QVAR, WHOLE) \
  QVAR.part.whole = WHOLE;\
  QVAR.part.fractional = 0;

typedef union q15_16 {
  int32_t full_value;
  struct part_tag {
    uint16_t fractional;
    int16_t whole;
  } part;
} q15_16t;

uint8_t clrscreencmd[] = {0x00, 0x03, 0x00, 0x40, 0xff, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00};
uint8_t clrpriocmd[] = {0x00, 0x03, 0x00, 0x40, 0x44, 0x00, 0x00, 0x00, 0x40, 0x05, 0x00, 0x00, 0x00};
uint8_t setcolorcmd[] = {0x81, 0xff, 0x85, 0x02, 0x00, 0x03, 0xD0, 0x07, 0x08, 0x00, 0x00, 0x00, 0x10, 0x08, 0x00, 0x00, 0x00};
uint8_t setcolorcmd2[] = {0x81, 0xff, 0x85, 0x02, 0x00, 0x03, 0xD0, 0x07, 0x1f, 0x00, 0x00, 0x01, 0x10, 0x08, 0x00, 0x00, 0x00};
uint8_t palettedata[] =  {0x00, 0x00, 0x00,
                          0x00, 0x00, 0x8A,
                          0x00, 0x8A, 0x00,
                          0x00, 0x8A, 0x8A,
                          0x8A, 0x00, 0x00,
                          0x8A, 0x00, 0x8A,
                          0x8A, 0x45, 0x00,
                          0x8A, 0x8A, 0x8A,
                          0x45, 0x45, 0x45,
                          0x45, 0x45, 0xCF,
                          0x45, 0xCF, 0x45,
                          0x45, 0xCF, 0xCF,
                          0xCF, 0x45, 0x45,
                          0xCF, 0x45, 0xCF,
                          0xCF, 0xCF, 0x45,
                          0xCF, 0xCF, 0xCF};

void savevic(void) {
  vic4cache[0] = VICIV.ctrl2;
  vic4cache[1] = VICIV.ctrla;
  vic4cache[2] = VICIV.ctrlb;
  vic4cache[3] = VICIV.ctrlc;
  vic4cache[4] = VICIV.linestep >> 8;
  vic4cache[5] = VICIV.linestep & 0xff;
  vic4cache[6] = VICIV.chrcount;
  vic4cache[7] = VICIV.palsel;
  vic4cache[8] = VICIV.scrnptr_mb;
  vic4cache[9] = VICIV.scrnptr_bnk;
  vic4cache[10] = VICIV.scrnptr_msb;
  vic4cache[11] = VICIV.scrnptr_lsb;
  vic4cache[12] = VICIV.colptr >> 8;
  vic4cache[13] = VICIV.colptr & 0xff;
}

void loadvic(void) {
  VICIV.ctrl2 = vic4cache[0];
  VICIV.ctrla = vic4cache[1];
  VICIV.ctrlb = vic4cache[2];
  VICIV.ctrlc = vic4cache[3];
  VICIV.linestep = (vic4cache[4] << 8) | vic4cache[5];
  VICIV.chrcount = vic4cache[6];
  VICIV.palsel = vic4cache[7];
  VICIV.scrnptr_mb = vic4cache[8];
  VICIV.scrnptr_bnk = vic4cache[9];
  VICIV.scrnptr_msb = vic4cache[10];
  VICIV.scrnptr_lsb = vic4cache[11];
  VICIV.colptr = (vic4cache[12] << 8) | vic4cache[13];
}

void gfx_print_parserline(uint8_t character) {
  static uint16_t parser_printpos = 422;
  uint16_t __far *screen_memory = (uint16_t __far *)0x012000;
  uint16_t __far *color_memory = (uint16_t __far *)0xff81000;
  if (character == '\r') {
    for (uint16_t i = 420; i < 500; i++) {
      screen_memory[i] = 0x0020;
      color_memory[i] = 0x0100;
    }
    screen_memory[420] = 0x003e;
    parser_printpos = 421;
  } else if (character == 0x14) {
    parser_printpos--;
    screen_memory[parser_printpos] = 0x0020;
  } else {
    if (character < 32) {
      character = character + 0x80;
    } else if (character < 64) {
      character = character + 0x00;
    } else if (character < 96) {
      character = character + 0xC0;
    } else if (character < 128) {
      character = character + 0xE0;
    } else if (character < 160) {
      character = character + 0x40;
    } else if (character < 192) {
      character = character + 0xC0;
    } else {
      character = character + 0x80;
    }
    screen_memory[parser_printpos] = character;
    parser_printpos++;
  }
}

int agi_q15round(q15_16t aNumber, int16_t dirn)
{
  int16_t floornum = aNumber.part.whole;
  int16_t ceilnum = aNumber.part.whole+1;

   if (dirn < 0)
      return ((aNumber.part.fractional <= 0x8042) ?
        floornum : ceilnum);
   return ((aNumber.part.fractional < 0x7fbe) ?
        floornum : ceilnum);
}

void gfx_plotput(uint8_t screen_num, uint16_t x, uint16_t y, uint8_t color) {
  uint8_t highpix = x & 1;
  uint8_t column = x >> 4;
  uint8_t offset = (x >> 1) & 0x07;
  uint16_t row = y << 3;
  volatile uint8_t __far *target_pixel;
  if (screen_num == 0) {
    target_pixel = graphics_memory[column] + offset + row;
  } else {
    target_pixel = prio_memory[column] + offset + row;
  }
  uint8_t curpix = *(target_pixel);
  if (highpix) {
       curpix = (curpix & 0x0f) | (color << 4);
  } else {
       curpix = (curpix & 0xf0) | (color & 0x0f);
  }
  *target_pixel = curpix;
}

uint8_t gfx_getprio(uint16_t x, uint16_t y) {
  uint8_t highpix = x & 1;
  uint8_t column = x >> 4;
  uint8_t offset = (x >> 1) & 0x07;
  uint16_t row = y << 3;
  volatile uint8_t __far *prio_start = prio_memory[column] + offset + row;
  if (highpix) {
    while (*prio_start < 0x40) {
      prio_start += 8;
    }
    return (*prio_start >> 4);
  } else {
    while ((*prio_start  & 0x0f) < 0x04) {
      prio_start += 8;
    }
    return (*prio_start & 0x0f);
  }
}

uint8_t gfx_get(uint8_t screen_num, uint16_t x, uint16_t y) {
  uint8_t highpix = x & 1;
  uint8_t column = x >> 4;
  uint8_t offset = (x >> 1) & 0x07;
  uint16_t row = y << 3;
  if (screen_num == 0) {
    uint8_t pixelval = *(graphics_memory[column] + offset + row); 
    if (highpix) {
      return (pixelval >> 4);
    } else {
      return (pixelval & 0x0f);
    }
  } else {
    uint8_t pixelval = *(prio_memory[column] + offset + row); 
    if (highpix) {
      return (pixelval >> 4);
    } else {
      return (pixelval & 0x0f);
    }
  }
}

void gfx_drawslowline(uint8_t screen_num, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t colour) {
   int16_t height, width;
   q15_16t x, y, dependent;
   int8_t increment;

   height = ((int16_t)y2 - y1);
   width = ((int16_t)x2 - x1);
   uint8_t absheight = abs(height);
   uint8_t abswidth = abs(width);
   if (abs(width) > abs(height)) {
      Q15_16_INT_TO_Q(x, x1);
      Q15_16_INT_TO_Q(y, y1);
      if (width > 0) {
        increment = 1;
      } else {
        increment = -1;
      }
      Q15_16_INT_TO_Q(dependent, height);
      dependent.full_value = (width  == 0 ? 0:(dependent.full_value/abswidth));
      for (; x.part.whole != x2; x.part.whole += increment) {
        int roundx = agi_q15round(x, increment);
        int roundy = agi_q15round(y, dependent.part.whole);
         gfx_plotput(screen_num, roundx, roundy, colour);
         y.full_value += dependent.full_value;
      }
      gfx_plotput(screen_num, x2, y2, colour);
   }
   else {
      Q15_16_INT_TO_Q(x, x1);
      Q15_16_INT_TO_Q(y, y1);
      if (height > 0) {
        increment = 1;
      } else {
        increment = -1;
      }
      Q15_16_INT_TO_Q(dependent, width);
      dependent.full_value = (height == 0 ? 0:(dependent.full_value/absheight));
      for (; y.part.whole!=y2; y.part.whole += increment) {
        uint8_t roundx = agi_q15round(x, dependent.part.whole);
        uint8_t roundy = agi_q15round(y, increment);
         gfx_plotput(screen_num, roundx, roundy, colour);
         x.full_value += dependent.full_value;
      }
      gfx_plotput(screen_num, x2,y2, colour);
   }
}

void gfx_drawfastline(uint8_t screen_num, int x1, int y1, int x2, int y2, unsigned char colour)
{
  gfx_drawslowline(screen_num, x1, y1, x2, y2, colour);
}

void gfx_setupmem(void) {
  uint16_t __far *screen_memory = (uint16_t __far *)0x012000;
  uint16_t __far *prio_screen_memory = (uint16_t __far *)0x012400;
  for (int y = 0; y < 25; y++) {
    int xoffset = 0;
    for (int x = 0; x < 10; x++) {
      *screen_memory = 0x1400 + xoffset + y;
      *prio_screen_memory = 0x1500 + xoffset + y;
      xoffset += 25;
      screen_memory = screen_memory + 1;
      prio_screen_memory = prio_screen_memory + 1;
    }
    screen_memory = screen_memory + 10;
    prio_screen_memory = prio_screen_memory + 10;
  }
  uint32_t offset = 0x50000;
  for (uint8_t x = 0; x < 10; x++) {
    graphics_memory[x] = (uint8_t __far *)offset;
    offset += 1600;
  }
  offset = 0x54000;
  for (uint8_t x = 0; x < 10; x++) {
    prio_memory[x] = (uint8_t __far *)offset;
    offset += 1600;
  }

  uint8_t palette_index = 16;
  for (int i = 0; i < 48; i += 3) {
    PALETTE.red[palette_index] = palettedata[i];
    PALETTE.green[palette_index] = palettedata[i+1];
    PALETTE.blue[palette_index] = palettedata[i+2];
    palette_index++;
  }

  DMA.dmahigh = (uint8_t)(((uint16_t)clrscreencmd) >> 8);
  DMA.etrig = (uint8_t)(((uint16_t)clrscreencmd) & 0xff);
  DMA.dmahigh = (uint8_t)(((uint16_t)clrpriocmd) >> 8);
  DMA.etrig = (uint8_t)(((uint16_t)clrpriocmd) & 0xff);
  DMA.dmahigh = (uint8_t)(((uint16_t)setcolorcmd) >> 8);
  DMA.etrig = (uint8_t)(((uint16_t)setcolorcmd) & 0xff);
  DMA.dmahigh = (uint8_t)(((uint16_t)setcolorcmd2) >> 8);
  DMA.etrig = (uint8_t)(((uint16_t)setcolorcmd2) & 0xff);

}

void gfx_showprio(void) {
    VICIV.scrnptr = 0x00012400;
}

void gfx_showgfx(void) {
    VICIV.scrnptr = 0x00012000;
}

void gfx_switchto(void) {
    savevic();
    
    gfx_setupmem();

    VICIV.ctrl2 = VICIV.ctrl2 | 0x10;
    VICIV.ctrla = VICIV.ctrla | VIC3_PAL_MASK;
    VICIV.ctrlb = VICIV.ctrlb & ~(VIC3_H640_MASK | VIC3_V400_MASK);
    VICIV.ctrlc = (VICIV.ctrlc & ~(VIC4_FCLRLO_MASK)) | (VIC4_FCLRHI_MASK | VIC4_CHR16_MASK);
    VICIV.linestep = 40;
    VICIV.chrcount = 20;
    VICIV.chrxscl = 60;

    VICIV.scrnptr = 0x00012000;
    VICIV.colptr = 0x1000;
}

void gfx_switchfrom(void) {
    loadvic();
}