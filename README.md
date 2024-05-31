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

## File Selection Screen

Use the up and down arrow keys to highlight a file or "New Composition". Use the enter key to select. tinydaw will load the file you selected or, if you choose New Composition it will create a file (it firstly tries to use music.mzk but if that isn't available will use music-1.mzk, music-2.mzk, etc)

## Music Screen

Because tinydaw necessarily has a lot of keys, in the music screen you can hold down the 'h' key to get a summary and what keys do.

tinydaw is inspired by Beepbox (https://www.beepbox.co/). Musical notes are placed into blocks that are assigned to tracks. By doing this we save file size as blocks of notes that repeat are only stored once and referenced by their number.

At the top of the screen are 8 tracks, that contain 16 numbers ( in the beginning all set to block 0 ). When the song is played the the player will play each block of notes in turn, with all tracks simultaneously. You can navigate the highlight around this area with the WASD keys. To increase the block number in a section use the 'e' key and to decrease it use the 'q' key. There are 100 blocks of notes available to use.

Below the track information is a long bar. In the center of this bar you will be able to read the tempo. You can increase the tempo with the Page Up key and decrease it with the Page Down key. If you do this while the music is currently playing it may sound a bit funny for a while as the system catches up. It is best to stop the music, change the tempo, and then start it again. But let's be honest, nobody's going to do that :)

The long bar represents the play loop. This enables you to work on small sections of your song at a time. By moving the highlight to a column in the track area and pressing 'z', you can set the start of the play loop. By moving it to a column and pressing 'x', you can set the end of the play loop.

Press the space bar to start playing, press it again to stop.

Use the F1 key to save your music file out to the .mzk file.

Use the F12 key to exit the tool. This does not save, so save your file first with F1 if you want to.

Once you have a music file playing, you may want to change the instruments. Press the 'i' key for the instruments screen.

## Instruments Screen

The instruments screen has eight instruments, one for each track. An instrument is just a RIVES waveform that the player does a few calculations on and adjusts to play the musical notes.

You can move up and down through the instruments with the 'w' and 's' keys.

Each instrument has 7 parameters. You can move the highlighter to them with the left and right arrow keys.

"Type" is the waveform from rives. You can choose from SINE, SQUARE, TRIANGLE, SAWTOOTH, NOISE, and PULSE .

"AMP" is the amplitude of the wave. You can think of this as the early maximum volume that the note goes to.

"SUS" is the sustain volume. This is the later part of the volume after the decay. Use a combination of AMP and SUS to balance the volume of your instrument/track to the others.

"A" is attack. This is how long it takes your note to reach the initial amplitude.

"D" is decay. How quickly it takes to reach the sustain volume.

"R" is release. How quickly afterthe sustain that the note gets to zero volume.

"C" is the cycle for the wave generation.

You can adjust these values by using the UP and DOWN arrows when you have highlighted them (for fine-grain control hold the SHIFT key down, but it should rarely be necessary). It's recommended not to set these to zero as it sounds a bit un-natural, but you can go very close if you want.

The player compensates for timing issues - if the note being played is shorter than the waveform then the player will add sustain in to make it last that long. If, however, it is longer, the player will adjust parameters proportionally to make sure it fits.

To return to the Music screen, use the 'm' key.

# Using music in your own games

Ensure that music.c, music.h and the saved .mzk file from tinydaw are in your game's folder.

Add the following to the top of your game's main .c file

`#include "music.h"`

Load the music somewhere early on, for example in main

`load_music("music.mzk");`

Regulary, for example in your main loop, call play_music

`play_music();`

Compile your game including the music.c file, for example:

`./rivemu -workspace -sdk -no-loading -no-window -exec gcc mygame.c music.c -o mygame -lriv`

Run it as normal.

`./rivemu -workspace -exec ./mygame`

If you're making an sqfs cartridge, ensure that your music .mzk file is included, for example:

`./rivemu -quiet -no-window -sdk -workspace -exec riv-mksqfs 0-start.sh mygame music.mzk mygame.sqfs -comp xz`

# Notes

C is far from my primary language, please feel free to submit a pull request with improvements or suggestions.

Yes there are a lot of keys to press to do stuff. It's necessary for this kind of tool, you get used to them quickly.

There is basic compression in the .mzk files that makes them a lot smaller. They could be smaller still but the compression is super compatible with run length encoding. If you make an sqfs file, it's a good idea to use the `-comp x` flag

The License is MIT, so you don't need to worry about including things in your games. You don't not need to include the license file in your cartridges.
