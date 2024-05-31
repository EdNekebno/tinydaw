// Load and play a tinydaw music file

#include <riv.h>
#include "music.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

unsigned short blocks[NUM_TRACKS][NUM_BLOCK_NOTES];  // Holds track block data
short notes[NUM_BLOCKS][128][NUM_BLOCK_NOTES];       // Holds the notes in each block
bool note_playing[NUM_BLOCKS][128][NUM_BLOCK_NOTES]; // Holds a flag if the note is currently playing
unsigned int target_bpm = 120;                       // The BPM to play the music at
unsigned long music_start_frame;                     // The frame number the music started playing at
bool music_playing = false;                          // Whether music is currently playing
riv_waveform_desc instruments[NUM_TRACKS];           // Instruments for each track
riv_waveform_desc waveforms[NUM_TRACKS][128];        // Holds the waveforms calculated for each track and note
unsigned int start_playing = 0;                      // Holds the start point for looping playback
unsigned int stop_playing = 15;                      // Holds the end point for looping playback

void load_music(char *filename)
{
    // Loads a .mzk file saved from tinydaw

    FILE *f = fopen(filename, "rb");

    // Load tempo
    fread(&target_bpm, sizeof(target_bpm), 1, f);

    // Load track instruments
    for (int v = 0; v < NUM_TRACKS; v++)
    {
        fread(&instruments[v], sizeof(instruments[v]), 1, f);
    }

    // Load what blocks the tracks are using
    for (int t = 0; t < NUM_TRACKS; t++)
    {
        for (int nm = 0; nm < NUM_BLOCK_NOTES; nm++)
        {
            fread(&blocks[t][nm], sizeof(blocks[t][nm]), 1, f);
        }
    }

    // Load the notes in a block taking into account
    // the compression markers used to reduce the file size
    for (int b = 0; b < NUM_BLOCKS; b++)
    {
        char p = 0;
        fread(&p, 1, 1, f);
        if (p == 1)
        {
            for (int n = 0; n < 128; n++)
            {
                char r = 0;
                fread(&r, 1, 1, f);
                if (r == 1)
                {
                    for (int nm = 0; nm < NUM_BLOCK_NOTES; nm++)
                    {
                        fread(&notes[b][n][nm], sizeof(notes[b][n][nm]), 1, f);
                    }
                }
            }
        }
    }

    fclose(f);
}

void play_music()
{
    // Plays the music (call load_music with the .mzk file from tinydaw first)
    if (music_playing == false)
    {
        // If we're not currently playing music then set some things up
        // ready to play.
        music_start_frame = riv->frame;
        music_playing = true;
        // Next find the first column in the music that contains no notes
        // at all. This is the point at which we should loop the music
        // back to the beginning.
        for (int b = 0; b < NUM_BLOCK_NOTES; b++)
        {
            bool contains_notes = false;
            for (int t = 0; t < NUM_TRACKS; t++)
            {
                short block_id = blocks[t][b];
                for (int q = 0; q < 128; q++)
                {
                    for (int r = 0; r < NUM_BLOCK_NOTES; r++)
                    {
                        if (notes[block_id][q][r] > 0)
                        {
                            contains_notes = true;
                            break;
                        }
                    }
                    if (contains_notes == true)
                        break;
                }
                if (contains_notes == true)
                    break;
            }
            if (contains_notes == false)
            {
                stop_playing = b - 1;
                break;
            }
        }
    }
    else
    {
        // If music is playing then we need to calculate the current beat
        // to look up the note to play it...
        int range = (stop_playing + 1);                                             // Range is the number of blocks to be played
        int range_beats = range * 16;                                               // The number of beats (quarter notes) over this range
        unsigned long frame_offset = riv->frame - music_start_frame;                // An offset as an indicator of time
        float seconds_passed = (float)frame_offset / (float)riv->target_fps;        // Seconds passed
        seconds_passed = seconds_passed * 2.0;                                      // Each of our notes is actually an eight note
        int beats_passed = (int)floor(seconds_passed * ((float)target_bpm / 60.0)); // The number of beats that have passed
        int beat_in_range = beats_passed % range_beats;                             // The current beat in the range being played
        // See if we need to play a note. To do this we need to know the current block column being played
        // and the current note column
        int current_playing_block_column = (int)floor(beat_in_range / 16);
        int current_playing_note_column = beat_in_range - (current_playing_block_column * 16);
        for (int p = 0; p < NUM_TRACKS; p++)
        {
            int this_block_id = blocks[p][current_playing_block_column];
            for (int qq = 0; qq < 128; qq++)
            {
                int thisnote = notes[this_block_id][qq][current_playing_note_column];
                if (thisnote > -1 && note_playing[this_block_id][qq][current_playing_note_column] == false)
                {
                    // There's a note to play that isn't already playing
                    // Copy a waveform to the waveform for this track and note
                    waveforms[p][qq].type = instruments[p].type;
                    waveforms[p][qq].attack = instruments[p].attack;
                    waveforms[p][qq].decay = instruments[p].decay;
                    waveforms[p][qq].sustain = 0;
                    waveforms[p][qq].release = instruments[p].release;
                    waveforms[p][qq].amplitude = instruments[p].amplitude;
                    waveforms[p][qq].sustain_level = instruments[p].sustain_level;
                    waveforms[p][qq].duty_cycle = instruments[p].duty_cycle;
                    // Calculate the frequency of the note
                    int nq = 120 - qq; // our offset
                    float nn = (float)nq;
                    nn = nn - 69;
                    float frequency = pow(2, (nn / 12)) * 440;
                    waveforms[p][qq].start_frequency = frequency;
                    waveforms[p][qq].end_frequency = frequency;
                    // Calculate the note length in seconds
                    // thisnote contains 1 (eight note) 2 (quarter note)....8
                    float note_secs = ((float)thisnote / 2.0) * (60.0 / (float)target_bpm);
                    float waveform_base_len = waveforms[p][qq].attack + waveforms[p][qq].decay + waveforms[p][qq].release;
                    if (waveform_base_len <= note_secs)
                    {
                        // If attack, decay, and release are less than the note duration than calculate the sustain
                        waveforms[p][qq].sustain = note_secs - waveform_base_len;
                    }
                    else
                    {
                        // The waveform is longer than we want to play. To avoid clipping let's shorten it.
                        float r = note_secs / waveform_base_len;
                        waveforms[p][qq].attack *= r;
                        waveforms[p][qq].decay *= r;
                        waveforms[p][qq].release *= r;
                    }
                    riv_waveform(&waveforms[p][qq]);
                    // Set the note playing flags so we don't start it again next frame!
                    note_playing[this_block_id][qq][current_playing_note_column] = true;
                }
            }
            // Clear previous note playing flags so they're ready for the next loop
            int prev_column = current_playing_note_column - 1;
            if (prev_column < 0)
            {
                prev_column = 15;
            }
            for (int qq = 0; qq < 128; qq++)
            {
                note_playing[this_block_id][qq][prev_column] = false;
            }
        }
    }
}

void stop_music()
{
    // Stop the music playing
    music_playing = false;
    // Clear note playing flags
    for (int block = 0; block < NUM_BLOCKS; block++)
    {
        for (int x = 0; x < 128; x++)
        {
            for (int y = 0; y < NUM_BLOCK_NOTES; y++)
            {
                note_playing[block][x][y] = false;
            }
        }
    }
}