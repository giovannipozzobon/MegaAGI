#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <calypsi/intrinsics6502.h>
#include <mega65.h>
#include <math.h>
#include "gfx.h"

/// Poke a byte to the given address
#define POKE(X, Y) (*(volatile uint8_t*)(X)) = Y
/// Poke two bytes to the given address
#define POKE16(X, Y) (*(volatile uint16_t*)(X)) = Y
/// Poke four bytes to the given address
#define POKE32(X, Y) (*(volatile uint32_t*)(X)) = Y
/// Peek a byte from the given address
#define PEEK(X) (*(volatile uint8_t*)(X))
/// Peek two bytes from the given address
#define PEEK16(X) (*(volatile uint16_t*)(X))
/// Peek four bytes from the given address
#define PEEK32(X) (*(volatile uint32_t*)(X))
#define ASCIIKEY (*(volatile uint8_t *)0xd610)

uint8_t vic4cache[15];
volatile uint8_t __far *graphics_memory[20];
volatile uint8_t __far *prio_memory[20];

struct __F018 {
  uint8_t dmalowtrig;
  uint8_t dmahigh;
  uint8_t dmabank;
  uint8_t enable018;
  uint8_t addrmb;
  uint8_t etrig;
};

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

/// DMA controller
#define DMA (*(volatile struct __F018 *)0xd700)

static uint8_t line_dmalist[256];
unsigned char ofs;
unsigned char slope_ofs, line_mode_ofs, cmd_ofs, count_ofs;
unsigned char src_ofs, dst_ofs;

uint8_t clrscreencmd[] = {0x00, 0x03, 0x00, 0x80, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00};
uint8_t clrpriocmd[] = {0x00, 0x03, 0x00, 0x80, 0x14, 0x00, 0x00, 0x00, 0x80, 0x05, 0x00, 0x00, 0x00};
uint8_t setcolorcmd[] = {0x81, 0xff, 0x85, 0x02, 0x00, 0x03, 0xD0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x10, 0x08, 0x00, 0x00, 0x00};
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
  vic4cache[14] = VICIV.chrxscl;
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
  VICIV.chrxscl = vic4cache[14];
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
  uint8_t column = x >> 3;
  uint8_t offset = x & 0x07;
  uint16_t row = y << 3;
  if (screen_num == 0) {
    *(graphics_memory[column] + offset + row) = color | 0x10;
  } else {
    *(prio_memory[column] + offset + row) = color | 0x10;
  }
}

uint8_t gfx_getprio(uint16_t x, uint16_t y) {
  uint8_t column = x >> 3;
  uint8_t offset = x & 0x07;
  uint16_t row = y << 3;
  volatile uint8_t __far *prio_start = prio_memory[column] + offset + row;
  while (*prio_start < 0x14) {
    prio_start += 8;
  }
  return (*prio_start & 0x0f);
}

uint8_t gfx_get(uint8_t screen_num, uint16_t x, uint16_t y) {
  uint8_t column = x >> 3;
  uint8_t offset = x & 0x07;
  uint16_t row = y << 3;
  if (screen_num == 0) {
    return (*(graphics_memory[column] + offset + row) & 0x0f);
  } else {
    return (*(prio_memory[column] + offset + row) & 0x0f);
  }
}

void gfx_debugcode(uint16_t dbga, uint16_t dbgb) {
  uint8_t hexchrs[16] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
  uint16_t __far *screen_memory = (uint16_t __far *)0x012000;
  uint16_t __far *color_memory = (uint16_t __far *)0xff81000;
  screen_memory[420] = hexchrs[((dbga >> 12) & 0x0f)];
  color_memory[420] = 0x0100;
  screen_memory[421] = hexchrs[((dbga >> 8) & 0x0f)];
  color_memory[421] = 0x0100;
  screen_memory[422] = hexchrs[((dbga >> 4) & 0x0f)];
  color_memory[422] = 0x0100;
  screen_memory[423] = hexchrs[((dbga >> 0) & 0x0f)];
  color_memory[423] = 0x0100;
  screen_memory[426] = hexchrs[((dbgb >> 12) & 0x0f)];
  color_memory[426] = 0x0100;
  screen_memory[427] = hexchrs[((dbgb >> 8) & 0x0f)];
  color_memory[427] = 0x0100;
  screen_memory[428] = hexchrs[((dbgb >> 4) & 0x0f)];
  color_memory[428] = 0x0100;
  screen_memory[429] = hexchrs[((dbgb >> 0) & 0x0f)];
  color_memory[429] = 0x0100;
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
  long addr;
  int temp, slope, dx, dy;

  // Ignore if we choose to draw a point
  if (x2 == x1 && y2 == y1) {
    gfx_plotput(screen_num, x1, y1, colour);
    return;
  }

  dx = x2 - x1;
  dy = y2 - y1;
  if (dx < 0)
    dx = -dx;
  if (dy < 0)
    dy = -dy;

  // Draw line from x1,y1 to x2,y2
  if (dx < dy) {
    // Y is major axis

    if (y2 < y1) {
      temp = x1;
      x1 = x2;
      x2 = temp;
      temp = y1;
      y1 = y2;
      y2 = temp;
    }

    // Use hardware divider to get the slope
    MATH.multina16 = dx;
    MATH.multinb16 = dy;

    // Wait 16 cycles
    while(MATHBUSY);

    // Slope is the most significant bytes of the fractional part
    // of the division result
    slope = PEEK(0xD76A) + (PEEK(0xD76B) << 8);

    // Put slope into DMA options
    line_dmalist[slope_ofs] = slope & 0xff;
    line_dmalist[slope_ofs + 2] = slope >> 8;

    // Load DMA dest address with the address of the first pixel
    uint8_t xshift = x1 >> 3;
    addr = 0x50000 + (y1 << 3) + (x1 & 7) + xshift * 64 * 25L;
    if (screen_num == 1) {
      addr += 0x8000;
    }
    line_dmalist[dst_ofs + 0] = addr & 0xff;
    line_dmalist[dst_ofs + 1] = addr >> 8;
    line_dmalist[dst_ofs + 2] = (addr >> 16) & 0xf;

    // Source is the colour
    line_dmalist[src_ofs] = colour | 0x10;

    // Count is number of pixels, i.e., dy.
    dy = dy+3;
    line_dmalist[count_ofs] = dy & 0xff;
    line_dmalist[count_ofs + 1] = dy >> 8;

    // Command is FILL
    line_dmalist[cmd_ofs] = 0x03;

    // Line mode active, major axis is Y
    line_dmalist[line_mode_ofs] = 0x80 + 0x40 + (((x2 - x1) < 0) ? 0x20 : 0x00);

    DMA.dmahigh = (uint8_t)(((uint16_t)line_dmalist) >> 8);
    DMA.etrig = (uint8_t)(((uint16_t)line_dmalist) & 0xff);
  }
  else {
    // X is major axis

    if (x2 < x1) {
      temp = x1;
      x1 = x2;
      x2 = temp;
      temp = y1;
      y1 = y2;
      y2 = temp;
    }

    // Use hardware divider to get the slope
    MATH.multina16 = dy;
    MATH.multinb16 = dx;

    // Wait 16 cycles
    while(MATHBUSY);

    // Slope is the most significant bytes of the fractional part
    // of the division result
    slope = PEEK(0xD76A) + (PEEK(0xD76B) << 8);
    if (dx == dy) slope = 0xffff;
    // Put slope into DMA options
    line_dmalist[slope_ofs] = slope & 0xff;
    line_dmalist[slope_ofs + 2] = slope >> 8;

    // Load DMA dest address with the address of the first pixel
    uint8_t xshift = x1 >> 3;
    addr = 0x50000 + (y1 << 3) + (x1 & 7) + xshift * 64 * 25;
    if (screen_num == 1) {
      addr += 0x8000;
    }
    line_dmalist[dst_ofs + 0] = addr & 0xff;
    line_dmalist[dst_ofs + 1] = addr >> 8;
    line_dmalist[dst_ofs + 2] = (addr >> 16) & 0xf;

    // Source is the colour
    line_dmalist[src_ofs] = colour | 0x10;

    // Count is number of pixels, i.e., dy.
    dx = dx+3;
    line_dmalist[count_ofs] = dx & 0xff;
    line_dmalist[count_ofs + 1] = dx >> 8;

    // Command is FILL
    line_dmalist[cmd_ofs] = 0x03;

    // Line mode active, major axis is X
    line_dmalist[line_mode_ofs] = 0x80 + 0x00 + (((y2 - y1) < 0) ? 0x20 : 0x00);

    DMA.dmahigh = (uint8_t)(((uint16_t)line_dmalist) >> 8);
    DMA.etrig = (uint8_t)(((uint16_t)line_dmalist) & 0xff);
  }
}

void setup_dma_linedraw(void) {
    // Set up common structure of the DMA list
    ofs = 0;
    // Screen layout is in vertical stripes, so we need only to setup the
    // X offset step.  64x25 =
    line_dmalist[ofs++] = 0x87;
    line_dmalist[ofs++] = (1600 - 8) & 0xff;
    line_dmalist[ofs++] = 0x88;
    line_dmalist[ofs++] = (1600 - 8) >> 8;
    line_dmalist[ofs++] = 0x8b;
    slope_ofs = ofs++; // remember where we have to put the slope in
    line_dmalist[ofs++] = 0x8c;
    ofs++;
    line_dmalist[ofs++] = 0x8f;
    line_mode_ofs = ofs++;
    line_dmalist[ofs++] = 0x0a; // F018A list format
    line_dmalist[ofs++] = 0x00; // end of options
    cmd_ofs = ofs++;            // command byte
    count_ofs = ofs;
    ofs += 2;
    src_ofs = ofs;
    ofs += 3;
    dst_ofs = ofs;
    ofs += 3;
    line_dmalist[ofs++] = 0x00; // modulo
    line_dmalist[ofs++] = 0x00;
}

void gfx_setupmem(void) {
  uint16_t __far *screen_memory = (uint16_t __far *)0x012000;
  uint16_t __far *prio_screen_memory = (uint16_t __far *)0x012400;
  for (int y = 0; y < 25; y++) {
    int xoffset = 0;
    for (int x = 0; x < 20; x++) {
      *screen_memory = 0x1400 + xoffset + y;
      *prio_screen_memory = 0x1600 + xoffset + y;
      xoffset += 25;
      screen_memory = screen_memory + 1;
      prio_screen_memory = prio_screen_memory + 1;
    }
  }

  uint32_t offset = 0x50000;
  for (uint8_t x = 0; x < 20; x++) {
    graphics_memory[x] = (uint8_t __far *)offset;
    offset += 1600;
  }

  offset = 0x58000;
  for (uint8_t x = 0; x < 20; x++) {
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
    setup_dma_linedraw();

    VICIV.ctrl2 = VICIV.ctrl2 | 0x10;
    VICIV.ctrla = VICIV.ctrla | VIC3_PAL_MASK;
    VICIV.ctrlb = VICIV.ctrlb & ~(VIC3_H640_MASK | VIC3_V400_MASK);
    VICIV.ctrlc = (VICIV.ctrlc & ~(VIC4_FCLRLO_MASK)) | (VIC4_FCLRHI_MASK | VIC4_CHR16_MASK);
    VICIV.rasline0 = VICIV.rasline0 | VIC4_PALNTSC_MASK;
    VICIV.linestep = 40;
    VICIV.chrcount = 20;
    VICIV.chrxscl = 60;

    VICIV.scrnptr = 0x00012000;
    VICIV.colptr = 0x1000;
}

void gfx_switchfrom(void) {
    loadvic();
}