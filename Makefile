VPATH = src

# Common source files
ASM_SRCS = simplefile.s irq.s
C_SRCS = main.c ncm.c pic.c volume.c sound.c view.c engine.c interrupt.c memmanage.c sprite.c
C1541 = c1541
INC = -I./include

# Object files
OBJS = $(ASM_SRCS:%.s=obj/%.o) $(C_SRCS:%.c=obj/%.o)
OBJS_DEBUG = $(ASM_SRCS:%.s=obj/%-debug.o) $(C_SRCS:%.c=obj/%-debug.o)

obj/%.o: %.s
	as6502 --target=mega65 --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%.o: %.c
	cc6502 --target=mega65 -O2 $(INC) --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%-debug.o: %.s
	as6502 --target=mega65 --debug --list-file=$(@:%.o=%.lst) -o $@ $<

obj/%-debug.o: %.c
	cc6502 --target=mega65 --debug --list-file=$(@:%.o=%.lst) -o $@ $<

agi.prg:  $(OBJS)
	ln6502 --target=mega65 mega65-agi.scm -o $@ $^  --output-format=prg --list-file=agi-mega65.lst

agi.elf: $(OBJS_DEBUG)
	ln6502 --target=mega65 mega65-agi.scm --debug -o $@ $^ --list-file=agi-debug.lst --semi-hosted

agi.d81: agi.prg
	$(C1541) -format "agi,a1" d81 agi.d81
	$(C1541) -attach agi.d81 -write agi.prg agi.c65
	$(C1541) -attach agi.d81 -write volumes/LOGDIR logdir,s
	$(C1541) -attach agi.d81 -write volumes/PICDIR picdir,s
	$(C1541) -attach agi.d81 -write volumes/SNDDIR snddir,s
	$(C1541) -attach agi.d81 -write volumes/VIEWDIR viewdir,s
	$(C1541) -attach agi.d81 -write volumes/VOL.0 vol.0,s
#	$(C1541) -attach agi.d81 -write volumes/VOL.1 vol.1,s
#	$(C1541) -attach agi.d81 -write volumes/VOL.2 vol.2,s

clean:
	-rm $(OBJS) $(OBJS:%.o=%.lst) $(OBJS_DEBUG) $(OBJS_DEBUG:%.o=%.lst)
	-rm agi.elf agi.prg agi-mega65.lst agi-debug.lst
