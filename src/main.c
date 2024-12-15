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
#include "memmanage.h"
#include "simplefile.h"
#include "engine.h"
#include "irq.h"

#define ASCIIKEY (*(volatile uint8_t *)0xd610)
#define PETSCIIKEY (*(volatile uint8_t *)0xd619)
#define VICREGS ((volatile uint8_t *)0xd000)
#define POKE(X, Y) (*(volatile uint8_t*)(X)) = Y

int main () {
  VICIV.bordercol = COLOR_BLACK;
  VICIV.screencol = COLOR_BLACK;
  VICIV.key = 0x47;
  VICIV.key = 0x53;
  simplewrite(0x0b);
  simplewrite(0x0e);
  simpleprint("AGI DEMO!\r");

  memmanage_init();
  
  simpleprint("LOADING VOLUME FILES...\r");
  load_volume_files();
  simpleprint("LOADING DIRECTORY FILES...\r");
  load_directory_files();

  hook_irq();

  run_loop();

  return 0;
}
