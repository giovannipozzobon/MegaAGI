This is a graphical/sound demo version only.

The parser is not present, nor is the logic interpreter, so there is no gameplay functionality.

To use this, place agi.prg on a D81 with the following files from a AGI game:
PICDIR
SNDDIR
VIEWDIR
VOL.0
VOL.1
...
VOL.n

When it starts up, it will load PIC 0, and VIEW 0. This is a valid case for Kings Quest 1.
You can change the picture with the command (must use lower case):
> pic n

You can change the view with the command:
> view n

You can play a sound with the command:
> sound n

Sound 0 is the Kings Quest 1 title music, and sound 60 is the Space Quest 1 title music.

AGI games can be purchased from GOG, and contain all the files broken out nicely.

The drawing commands 0xF9 and 0xFA are not implemented, this will break a couple of games.

Larger VIEWs will flicker as they are moved around. Optimization is still required.
Also, I am using NCM mode, so a back-buffer system will come at some point.
