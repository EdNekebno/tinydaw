# tinydaw
tinydaw is a tiny, in emulator, music composer tool for the rives.io fantasy console. In combination with the music.c and music.h files, it allows you to easily create and add music to your rives games.

# Getting started

Install rivemu and the sdk (https://docs.rives.io/docs/riv/getting-started/#installing)

Install the rives SDK (https://docs.rives.io/docs/riv/developing-cartridges#installing-the-riv-sdk)

Place tinydaw.c , music.c, and music.h in the same folder as rivemu and the sdk

Compile tinydaw with

`./rivemu -workspace -sdk -no-loading -no-window -exec gcc tinydaw.c -o tinydaw -lriv`

tinydaw runs only from rivemu with the -workspace flag set, so that it can save the music file (.mzk) to your local machine.

`./rivemu -workspace -sdk -no-loading -exec ./tinydaw`

# Creating Music

TODO

# Using music in your own games

Ensure that music.c, music.h and the saved .mzk file from tinydaw are in your game's folder.

Add the following to the top of your game's main .c file

`#include "music.h"

Compile your game including the music.c file, for example:

`./rivemu -workspace -sdk -no-loading -no-window -exec gcc mygame.c music.c -o mygame -lriv`

Run it as normal.

`./rivemu -workspace -exec ./mygame`

If you're making an sqfs cartridge, ensure that your music .mzk file is included, for example:

`./rivemu -quiet -no-window -sdk -workspace -exec riv-mksqfs 0-start.sh mygame music.mzk mygame.sqfs -comp xz`

# Notes

C is far from my primary language, please feel free to submit a pull request with improvements or suggestions
