/* Tinydaw is a beepbox inspired music creation tool
   for generating music for RIVES.io games
*/

#include <riv.h>
#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define NUM_TRACKS 8
#define NUM_BLOCKS 99
#define NUM_BLOCK_NOTES 16

unsigned short blocks[NUM_TRACKS][NUM_BLOCK_NOTES]; // Holds track block data
unsigned int start_playing = 0; // Holds the start point for looping playback
unsigned int stop_playing = 15; // Holds the end point for looping playback
int selected_track = 0;
int selected_block = 0;
short notes[NUM_BLOCKS][128][NUM_BLOCK_NOTES];
bool note_playing[NUM_BLOCKS][128][NUM_BLOCK_NOTES];
unsigned int note_row_from = 50; // Display notes from
unsigned int note_row_to = 70; // Display notes to
int selected_note_row = 0;
int selected_note_col = 0;
unsigned int target_bpm = 120;
unsigned long music_start_frame;
bool music_playing = false;
int instrument_mode_track = 0;
int instrument_mode_item = 0;
riv_waveform_desc instruments[NUM_TRACKS];
riv_waveform_desc waveforms[NUM_TRACKS][128];
char *working_file = "music.mzk";
char *files[32];
int max_file = 0;
int highlighted_file = -1;

riv_waveform_desc saved_sfx = {
    .type = RIV_WAVEFORM_TRIANGLE,
    .attack = 0.025f,
    .decay = 0.075f,
    .sustain = 0.125f,
    .release = 0.120f,
    .start_frequency = 1760,
    .end_frequency = 1760,
    .amplitude = 0.25f,
    .sustain_level = 0.3f,
    .duty_cycle = 0.5
};

enum screenmodes { MUSIC,
                   INSTRUMENT,
                   FILE_OPEN };

int screenmode = FILE_OPEN;

void help_mode_file_loop()
{
    riv_draw_text("FILE SELECTION SCREEN", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 1, 1, RIV_COLOR_GREEN);
    riv_draw_text("Choose to make a 'NEW COMPOSITION' or", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 15, 1, RIV_COLOR_WHITE);
    riv_draw_text("select an existing file to edit.", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 25, 1, RIV_COLOR_WHITE);
    riv_draw_text("Use the up/down arrows to move the", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 35, 1, RIV_COLOR_WHITE);
    riv_draw_text("selection highlighter. Enter to select.", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 45, 1, RIV_COLOR_WHITE);
    riv_draw_text("Tinydaw uses a custom .mzk file format.", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 65, 1, RIV_COLOR_WHITE);
    riv_draw_text("You can put these files in the .sqfs", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 75, 1, RIV_COLOR_WHITE);
    riv_draw_text("for your games and use the game code", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 85, 1, RIV_COLOR_WHITE);
    riv_draw_text("to play them.", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 95, 1, RIV_COLOR_WHITE);
}

void help_mode_music_loop()
{
    riv_draw_text("MUSIC SCREEN", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 1, 1, RIV_COLOR_GREEN);
    riv_draw_text("Here you make your compositions. Notes are", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 15, 1, RIV_COLOR_WHITE);
    riv_draw_text("collected in groups called blocks. At the", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 25, 1, RIV_COLOR_WHITE);
    riv_draw_text("top of the screen are eight tracks and the", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 35, 1, RIV_COLOR_WHITE);
    riv_draw_text("blocks they play.", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 45, 1, RIV_COLOR_WHITE);
    riv_draw_text("Use WASD to move the highlighter around", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 65, 1, RIV_COLOR_WHITE);
    riv_draw_text("tracks. Use Q/E to increase", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 75, 1, RIV_COLOR_WHITE);
    riv_draw_text("and decrease the block number played at", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 85, 1, RIV_COLOR_WHITE);
    riv_draw_text("that point. Z or X will set the start and", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 95, 1, RIV_COLOR_WHITE);
    riv_draw_text("end points of the play loop to the current", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 105, 1, RIV_COLOR_WHITE);
    riv_draw_text("highlighted column (so you can play back", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 115, 1, RIV_COLOR_WHITE);
    riv_draw_text("sections). SPACE to start/stop play.", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 125, 1, RIV_COLOR_WHITE);
    riv_draw_text("PAGE UP/PAGE DOWN to adjust the tempo.", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 135, 1, RIV_COLOR_WHITE);
    riv_draw_text("At the bottom of the screen are the notes", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 155, 1, RIV_COLOR_WHITE);
    riv_draw_text("for the block number currently highlighted", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 165, 1, RIV_COLOR_WHITE);
    riv_draw_text("in the tracks. Use the arrow keys to move", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 175, 1, RIV_COLOR_WHITE);
    riv_draw_text("the selection highlight. 1 - 8 keys to", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 185, 1, RIV_COLOR_WHITE);
    riv_draw_text("insert a note. Delete key to delete a note.", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 195, 1, RIV_COLOR_WHITE);
    riv_draw_text("Use I to enter the instrument editor.", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 215, 1, RIV_COLOR_WHITE);
    riv_draw_text("F1 will save the music file to workspace", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 235, 1, RIV_COLOR_WHITE);
    riv_draw_text("F12 exits (save first if you want!)", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 245, 1, RIV_COLOR_WHITE);
}

void help_mode_instrument_loop()
{
    riv_draw_text("INSTRUMENT SCREEN", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 1, 1, RIV_COLOR_GREEN);
    riv_draw_text("This screen defines how each track sounds.", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 15, 1, RIV_COLOR_WHITE);
    riv_draw_text("There are 8 instruments for the 8 tracks.", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 25, 1, RIV_COLOR_WHITE);
    riv_draw_text("Use W/S to move up/down the track list.", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 35, 1, RIV_COLOR_WHITE);
    riv_draw_text("Each track has multiple parameters.", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 45, 1, RIV_COLOR_WHITE);
    riv_draw_text("Use the left and right arrows to move", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 65, 1, RIV_COLOR_WHITE);
    riv_draw_text("the selection. The up/down arrows will", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 75, 1, RIV_COLOR_WHITE);
    riv_draw_text("changed the selection value. For finer", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 85, 1, RIV_COLOR_WHITE);
    riv_draw_text("control, hold SHIFT with UP/Down.", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 95, 1, RIV_COLOR_WHITE);
    riv_draw_text("Use M to return to the music screen.", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1, 115, 1, RIV_COLOR_WHITE);
}

void init()
{
    // Clear the tracks and blocks
    for (int track = 0; track < NUM_TRACKS; track++) {
        for (int block = 0; block < NUM_BLOCK_NOTES; block++) {
            blocks[track][block] = 0;
        }
        instruments[track].type = RIV_WAVEFORM_PULSE;
        instruments[track].attack = 0.15f;
        instruments[track].decay = 0.2f;
        instruments[track].sustain = 0.0f;
        instruments[track].release = 0.25f;
        instruments[track].amplitude = 0.25f;
        instruments[track].sustain_level = 0.3;
        instruments[track].duty_cycle = 0.2;
    }
    for (int block = 0; block < NUM_BLOCKS; block++) {
        for (int x = 0; x < 128; x++) {
            for (int y = 0; y < NUM_BLOCK_NOTES; y++) {
                notes[block][x][y] = -1000;
                note_playing[block][x][y] = false;
            }
        }
    }
    // Turn on key tracking
    riv->tracked_keys[RIV_KEYCODE_W] = true;
    riv->tracked_keys[RIV_KEYCODE_A] = true;
    riv->tracked_keys[RIV_KEYCODE_S] = true;
    riv->tracked_keys[RIV_KEYCODE_D] = true;
    riv->tracked_keys[RIV_KEYCODE_Z] = true;
    riv->tracked_keys[RIV_KEYCODE_X] = true;
    riv->tracked_keys[RIV_KEYCODE_E] = true;
    riv->tracked_keys[RIV_KEYCODE_Q] = true;
    riv->tracked_keys[RIV_KEYCODE_1] = true;
    riv->tracked_keys[RIV_KEYCODE_2] = true;
    riv->tracked_keys[RIV_KEYCODE_3] = true;
    riv->tracked_keys[RIV_KEYCODE_4] = true;
    riv->tracked_keys[RIV_KEYCODE_5] = true;
    riv->tracked_keys[RIV_KEYCODE_6] = true;
    riv->tracked_keys[RIV_KEYCODE_7] = true;
    riv->tracked_keys[RIV_KEYCODE_8] = true;
    riv->tracked_keys[RIV_KEYCODE_SPACE] = true;
    riv->tracked_keys[RIV_KEYCODE_DELETE] = true;
    riv->tracked_keys[RIV_KEYCODE_I] = true;
    riv->tracked_keys[RIV_KEYCODE_M] = true;
    riv->tracked_keys[RIV_KEYCODE_LEFT_SHIFT] = true;
    riv->tracked_keys[RIV_KEYCODE_PAGE_UP] = true;
    riv->tracked_keys[RIV_KEYCODE_PAGE_DOWN] = true;
    riv->tracked_keys[RIV_KEYCODE_F1] = true;
    riv->tracked_keys[RIV_KEYCODE_F12] = true;
    riv->tracked_keys[RIV_KEYCODE_ENTER] = true;
    riv->tracked_keys[RIV_KEYCODE_H] = true;
}

void save_music()
{
    FILE *f = fopen(working_file, "wb");

    // Save tempo
    fwrite(&target_bpm, sizeof(target_bpm), 1, f);

    // Save track instruments
    for (int v = 0; v < NUM_TRACKS; v++)
    {
        fwrite(&instruments[v], sizeof(instruments[v]), 1, f);
    }

    // Save what blocks the tracks are using
    for (int t = 0; t < NUM_TRACKS; t++) {
        for (int nm = 0; nm < NUM_BLOCK_NOTES; nm++) {
            fwrite(&blocks[t][nm], sizeof(blocks[t][nm]), 1, f);
        }
    }

    // Save the notes in a block
    for (int b = 0; b < NUM_BLOCKS; b++) {
        bool block_has_notes = false;
        for (int n = 0; n < 128; n++) {
            for (int nm = 0; nm < NUM_BLOCK_NOTES; nm++) {
                if (notes[b][n][nm] != -1000) {
                    block_has_notes = true;
                }
            }
        }
        // Output a byte indicating whether the block notes are present
        char p = 0;
        if (block_has_notes == false) {
            fwrite(&p, 1, 1, f);
        } else {
            p = 1;
            fwrite(&p, 1, 1, f);
        }
        if (block_has_notes == true) {
            for (int n = 0; n < 128; n++)
            {
                bool row_has_notes = false;
                for (int nm = 0; nm < NUM_BLOCK_NOTES; nm++)
                {
                    if (notes[b][n][nm] != -1000) {
                        row_has_notes = true;
                    }
                }
                if (row_has_notes == false) {
                    p = 0;
                    fwrite(&p, 1, 1, f);
                } else {
                    p = 1;
                    fwrite(&p, 1, 1, f);
                    for (int nm = 0; nm < NUM_BLOCK_NOTES; nm++)
                    {
                        fwrite(&notes[b][n][nm], sizeof(notes[b][n][nm]), 1, f);
                    }
                }
            }
        }
    }

    fclose(f);
    riv_waveform(&saved_sfx);
}

void load_music()
{
    FILE *f = fopen(working_file, "rb");

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

    // Load the notes in a block
    for (int b = 0; b < NUM_BLOCKS; b++)
    {
        char p = 0;
        fread(&p, 1, 1, f);
        if (p == 1) {
            for (int n = 0; n < 128; n++)
            {
                char r = 0;
                fread(&r, 1, 1, f);
                if (r == 1) {
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

void check_rivemu_workspace() {
    // This tool should be run in rivemu with the workspace flag
    // with something like...
    // rivemu -workspace -exec tinydaw
    char check_for[260] = "rivemu";
    bool found = false;
    max_file = 0;
    DIR *d;
    struct dirent *dir;
    d = opendir("/workspace");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (strcmp(dir->d_name, check_for) == 0) {
                found = true;
            }
            if (dir->d_name[0] != '.') {
                if (strstr(dir->d_name, ".mzk") > 0) {
                    files[max_file] = calloc(1, 1000);
                    snprintf(files[max_file], 200, "%s", dir->d_name);
                    max_file++;
                }
            }
        }
        closedir(d);
    }

    if (found == false) {
        riv_printf("Run the tool with the workspace parameter. e.g. ./rivemu -workspace -exec tinydaw\n");
        riv->quit_frame = riv->frame + 1;
    }

}

void check_keys_music() {
    if (riv->keys[RIV_KEYCODE_F1].press) {
        // Save the working file.
        save_music();
    }
    if (riv->keys[RIV_KEYCODE_F12].press) {
        // Exit
        for (int i = 0; i < max_file; i++) {
            free(files[i]);
        }
        riv->quit_frame = riv->frame + 1;
    }
    // WASD should move in the track blocks section
    if (riv->keys[RIV_KEYCODE_W].press)
    {
        if (selected_track > 0)
        {
            selected_track--;
        }
    }
    if (riv->keys[RIV_KEYCODE_S].press)
    {
        if (selected_track < NUM_TRACKS - 1)
        {
            selected_track++;
        }
    }
    if (riv->keys[RIV_KEYCODE_A].press)
    {
        if (selected_block > 0)
        {
            selected_block--;
        }
    }
    if (riv->keys[RIV_KEYCODE_D].press)
    {
        if (selected_block < NUM_BLOCK_NOTES - 1)
        {
            selected_block++;
        }
    }
    // Z will set the loop start to the current block column
    if (riv->keys[RIV_KEYCODE_Z].press)
    {
        if (selected_block <= stop_playing) {
            start_playing = selected_block;
        }
    }
    // X will set the loop end to the current block column
    if (riv->keys[RIV_KEYCODE_X].press)
    {
        if (selected_block >= start_playing)
        {
            stop_playing = selected_block;
        }
    }
    // E will increase the block number
    if (riv->keys[RIV_KEYCODE_E].press) {
        if (blocks[selected_track][selected_block] < NUM_BLOCKS) {
            blocks[selected_track][selected_block]++;
        }
    }
    // Q will decrease the block number
    if (riv->keys[RIV_KEYCODE_Q].press)
    {
        if (blocks[selected_track][selected_block] > 0)
        {
            blocks[selected_track][selected_block]--;
        }
    }
    // Arrow keys will move the note selection
    if (riv->keys[RIV_GAMEPAD_UP].press) {
        if (selected_note_row > 0) {
            selected_note_row--;
        } else {
            if (note_row_from > 0) {
                note_row_from--;
                note_row_to--;
            }
        }
    }
    if (riv->keys[RIV_GAMEPAD_DOWN].press) {
        if (selected_note_row < 19) {
            selected_note_row++;
        } else {
            note_row_from++;
            note_row_to++;
        }
    }
    if (riv->keys[RIV_GAMEPAD_LEFT].press)
    {
        if (selected_note_col > 0) {
            selected_note_col--;
        }
    }
    if (riv->keys[RIV_GAMEPAD_RIGHT].press)
    {
        if (selected_note_col < 15)
        {
            selected_note_col++;
        }
    }
    // Number keys select note lengths
    if (riv->keys[RIV_KEYCODE_1].press && notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col] == -1000)
    {
        notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col] = 1;
    }
    if (riv->keys[RIV_KEYCODE_2].press && selected_note_col <= 14 && notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col] == -1000)
    {
        notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col] = 2;
        for (int i = 1; i < 2; i++)
        {
            notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col + i] = -2;
        }
    }
    if (riv->keys[RIV_KEYCODE_3].press && selected_note_col <= 13 && notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col] == -1000)
    {
        notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col] = 3;
        for (int i = 1; i < 3; i++)
        {
            notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col + i] = -3;
        }
    }
    if (riv->keys[RIV_KEYCODE_4].press && selected_note_col <= 12 && notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col] == -1000)
    {
        notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col] = 4;
        for (int i = 1; i < 4; i++)
        {
            notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col + i] = -4;
        }
    }
    if (riv->keys[RIV_KEYCODE_5].press && selected_note_col <= 11 && notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col] == -1000)
    {
        notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col] = 5;
        for (int i = 1; i < 5; i++)
        {
            notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col + i] = -5;
        }
    }
    if (riv->keys[RIV_KEYCODE_6].press && selected_note_col <= 10 && notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col] == -1000)
    {
        notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col] = 6;
        for (int i = 1; i < 6; i++)
        {
            notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col + i] = -6;
        }
    }
    if (riv->keys[RIV_KEYCODE_7].press && selected_note_col <= 9 && notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col] == -1000)
    {
        notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col] = 7;
        for (int i = 1; i < 7; i++)
        {
            notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col + i] = -7;
        }
    }
    if (riv->keys[RIV_KEYCODE_8].press && selected_note_col <= 8 && notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col] == -1000)
    {
        notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col] = 8;
        for (int i = 1; i < 8; i++)
        {
            notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col + i] = -8;
        }
    }
    // Delete key deletes a note
    if (riv->keys[RIV_KEYCODE_DELETE].press)
    {
        int offset = 0;
        int nn = notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col - offset];
        while (nn < 0 && nn != -1000) {
            nn = notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col - offset];
            if (nn <0 && nn != -1000) {
                offset++;
            }
        }
        if (nn > 0) {
            for (int x = 0; x < nn; x++) {
                notes[blocks[selected_track][selected_block]][note_row_from + selected_note_row][selected_note_col + x - offset] = -1000;
            }
        }
    }
    // Space toggles music playing
    if (riv->keys[RIV_KEYCODE_SPACE].press) {
        if (music_playing == true) {
            music_playing = false;
        } else {
            music_playing = true;
            music_start_frame = riv->frame;
        }
    }
    // I switches to instrument mode
    if (riv->keys[RIV_KEYCODE_I].press) {
        screenmode = INSTRUMENT;
    }
    // Page up and down to change the tempo
    if (riv->keys[RIV_KEYCODE_PAGE_UP].press) {
        target_bpm += 5;
        if (target_bpm > 900) {
            target_bpm = 900;
        }
    }
    if (riv->keys[RIV_KEYCODE_PAGE_DOWN].press)
    {
        target_bpm -= 5;
        if (target_bpm < 20) {
            target_bpm = 20;
        }
    }
}

void check_instrument_keys() {
    if (riv->keys[RIV_KEYCODE_F1].press)
    {
        // Save the working file.
        save_music();
    }
    if (riv->keys[RIV_KEYCODE_F12].press)
    {
        // Exit
        for (int i = 0; i < max_file; i++)
        {
            free(files[i]);
        }
        riv->quit_frame = riv->frame + 1;
    }
    if (riv->keys[RIV_GAMEPAD_RIGHT].press) {
        instrument_mode_item++;
        if (instrument_mode_item > 6) {
            instrument_mode_item = 0;
        }
    }
    if (riv->keys[RIV_GAMEPAD_LEFT].press)
    {
        instrument_mode_item--;
        if (instrument_mode_item < 0)
        {
            instrument_mode_item = 6;
        }
    }
    if (riv->keys[RIV_KEYCODE_W].press) {
        instrument_mode_track--;
        if (instrument_mode_track < 0) {
            instrument_mode_track = NUM_TRACKS - 1;
        }
    }
    if (riv->keys[RIV_KEYCODE_S].press) {
        instrument_mode_track++;
        if (instrument_mode_track > NUM_TRACKS - 1) {
            instrument_mode_track = 0;
        }
    }
    if (riv->keys[RIV_GAMEPAD_UP].press && instrument_mode_item == 0) {
        switch (instruments[instrument_mode_track].type) {
            case RIV_WAVEFORM_PULSE:
                instruments[instrument_mode_track].type = RIV_WAVEFORM_NOISE;
                break;
            case RIV_WAVEFORM_NOISE:
                instruments[instrument_mode_track].type = RIV_WAVEFORM_SAWTOOTH;
                break;
            case RIV_WAVEFORM_SAWTOOTH:
                instruments[instrument_mode_track].type = RIV_WAVEFORM_TRIANGLE;
                break;
            case RIV_WAVEFORM_TRIANGLE:
                instruments[instrument_mode_track].type = RIV_WAVEFORM_SQUARE;
                break;
            case RIV_WAVEFORM_SQUARE:
                instruments[instrument_mode_track].type = RIV_WAVEFORM_SINE;
                break;
            case RIV_WAVEFORM_SINE:
                instruments[instrument_mode_track].type = RIV_WAVEFORM_PULSE;
                break;
        }
    }
    if (riv->keys[RIV_GAMEPAD_DOWN].press && instrument_mode_item == 0)
    {
        switch (instruments[instrument_mode_track].type)
        {
        case RIV_WAVEFORM_PULSE:
            instruments[instrument_mode_track].type = RIV_WAVEFORM_SINE;
            break;
        case RIV_WAVEFORM_NOISE:
            instruments[instrument_mode_track].type = RIV_WAVEFORM_PULSE;
            break;
        case RIV_WAVEFORM_SAWTOOTH:
            instruments[instrument_mode_track].type = RIV_WAVEFORM_NOISE;
            break;
        case RIV_WAVEFORM_TRIANGLE:
            instruments[instrument_mode_track].type = RIV_WAVEFORM_SAWTOOTH;
            break;
        case RIV_WAVEFORM_SQUARE:
            instruments[instrument_mode_track].type = RIV_WAVEFORM_TRIANGLE;
            break;
        case RIV_WAVEFORM_SINE:
            instruments[instrument_mode_track].type = RIV_WAVEFORM_SQUARE;
            break;
        }
    }
    if (riv->keys[RIV_GAMEPAD_UP].down && instrument_mode_item > 0) {
        float change = 0.001;
        if (riv->keys[RIV_KEYCODE_LEFT_SHIFT].down) {
            change = 0.00001;
        }
        switch (instrument_mode_item) {
            case 1:
                instruments[instrument_mode_track].amplitude += change;
                if (instruments[instrument_mode_track].amplitude > 1) {
                    instruments[instrument_mode_track].amplitude = 1;
                }
                break;
            case 2:
                instruments[instrument_mode_track].sustain_level += change;
                if (instruments[instrument_mode_track].sustain_level > 1)
                {
                    instruments[instrument_mode_track].sustain_level = 1;
                }
                break;
            case 3:
                instruments[instrument_mode_track].attack += change;
                if (instruments[instrument_mode_track].attack > 1)
                {
                    instruments[instrument_mode_track].attack = 1;
                }
                break;
            case 4:
                instruments[instrument_mode_track].decay += change;
                if (instruments[instrument_mode_track].decay > 1)
                {
                    instruments[instrument_mode_track].decay = 1;
                }
                break;
            case 5:
                instruments[instrument_mode_track].release += change;
                if (instruments[instrument_mode_track].release > 1)
                {
                    instruments[instrument_mode_track].release = 1;
                }
                break;
            case 6:
                instruments[instrument_mode_track].duty_cycle += change;
                if (instruments[instrument_mode_track].duty_cycle > 1)
                {
                    instruments[instrument_mode_track].duty_cycle = 1;
                }
                break;
        }
    }
    if (riv->keys[RIV_GAMEPAD_DOWN].down && instrument_mode_item > 0)
    {
        float change = 0.001;
        if (riv->keys[RIV_KEYCODE_LEFT_SHIFT].down)
        {
            change = 0.00001;
        }
        switch (instrument_mode_item)
        {
        case 1:
            instruments[instrument_mode_track].amplitude -= change;
            if (instruments[instrument_mode_track].amplitude < 0)
            {
                instruments[instrument_mode_track].amplitude = 0;
            }
            break;
        case 2:
            instruments[instrument_mode_track].sustain_level -= change;
            if (instruments[instrument_mode_track].sustain_level < 0)
            {
                instruments[instrument_mode_track].sustain_level = 0;
            }
            break;
        case 3:
            instruments[instrument_mode_track].attack -= change;
            if (instruments[instrument_mode_track].attack < 0)
            {
                instruments[instrument_mode_track].attack = 0;
            }
            break;
        case 4:
            instruments[instrument_mode_track].decay -= change;
            if (instruments[instrument_mode_track].decay < 0)
            {
                instruments[instrument_mode_track].decay = 0;
            }
            break;
        case 5:
            instruments[instrument_mode_track].release -= change;
            if (instruments[instrument_mode_track].release < 0)
            {
                instruments[instrument_mode_track].release = 0;
            }
            break;
        case 6:
            instruments[instrument_mode_track].duty_cycle -= change;
            if (instruments[instrument_mode_track].duty_cycle < 0)
            {
                instruments[instrument_mode_track].duty_cycle = 0;
            }
            break;
        }
    }

    if (riv->keys[RIV_KEYCODE_M].press) {
        screenmode = MUSIC;
    }
}

void check_file_keys() {
    if (riv->keys[RIV_GAMEPAD_UP].press) {
        highlighted_file--;
        if (highlighted_file < -1) {
            highlighted_file = -1;
        }
    }
    if (riv->keys[RIV_GAMEPAD_DOWN].press)
    {
        highlighted_file++;
        if (highlighted_file > max_file - 1)
        {
            highlighted_file = max_file - 1;
        }
    }
    if (riv->keys[RIV_KEYCODE_ENTER].press) {
        if (highlighted_file == -1) {
            working_file = "music.mzk";
            for (int i = 0; i < max_file; i++) {
                if (strcmp(files[i], working_file) == 0) {
                    // File already exists
                    for (int k = 1; k < 10000; k++) {
                        char* new_file = calloc(1,1000);
                        snprintf(new_file, 100, "music-%d.mzk", k);
                        bool found = false;
                        for (int q = 0; q < max_file; q++) {
                            if (strcmp(files[q], new_file) == 0) {
                                found = true;
                            }
                        }
                        if (found == false) {
                            working_file = new_file;
                            break;
                        }
                    }
                    break;
                }
            }
        } else {
            working_file = files[highlighted_file];
            load_music();
        }
        screenmode = MUSIC;
    }
}

void midi_number_to_name(int midi_num, char *ret) {
    const char notes[12][4] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = ((midi_num - 12) / 12);
    snprintf(ret, 6, "%s%d", notes[midi_num % 12], octave);
}

void music_mode_loop() {

    if (riv->keys[RIV_KEYCODE_H].down)
    {
        help_mode_music_loop();
        return;
    }

    check_keys_music();

    // Draw the tracks filled with the blocks
    riv_draw_rect_fill((selected_block * 16), selected_track * 10, 16, 10, RIV_COLOR_GREY);
    char buff[5];
    for (int track = 0; track < NUM_TRACKS; track++)
    {
        for (int block = 0; block < NUM_BLOCK_NOTES; block++)
        {
            if (blocks[track][block] > NUM_BLOCKS)
            {
                blocks[track][block] = 0;
            }
            snprintf(buff, 5, "%d", blocks[track][block]);
            riv_draw_text(buff, RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, 7 + (block * 16), 5 + (track * 10), 1, RIV_COLOR_WHITE);
        }
    }

    // Controls block background
    riv_draw_rect_fill(0, 1 + NUM_TRACKS * 10, 256, 13, RIV_COLOR_DARKRED);

    // Draw the play loop indicator
    riv_draw_rect_fill((start_playing * 16) + 1, 5 + NUM_TRACKS * 10, ((stop_playing - start_playing + 1) * 16) - 2, 6, RIV_COLOR_LIGHTPEACH);
    riv_draw_rect_line((start_playing * 16) + 1, 5 + NUM_TRACKS * 10, ((stop_playing - start_playing + 1) * 16) -2, 6, RIV_COLOR_WHITE);

    // Write the target bpm
    char tbpm[20];
    snprintf(tbpm, 20, "%d", target_bpm);
    riv_draw_rect_fill(128 - 8, 4 + NUM_TRACKS * 10, 17, 7, RIV_COLOR_GREY);
    riv_draw_text(tbpm, RIV_SPRITESHEET_FONT_3X5, RIV_CENTER, 128, 7 + (NUM_TRACKS * 10), 1, RIV_COLOR_BLUE);

    // Draw the notes
    int already_outlined = 0;
    for (int qq = note_row_from; qq < note_row_to; qq++)
    {
        char *notename = calloc(1,7);;
        midi_number_to_name(120 - qq, notename);
        for (int qc = 0; qc < 16; qc++)
        {
            if (notes[blocks[selected_track][selected_block]][qq][qc] == -1000) {
                if (notename[1] == '#')
                {
                    riv_draw_rect_fill((qc * 16), (15 + NUM_TRACKS * 10) + ((qq - note_row_from) * 8), 16, 8, RIV_COLOR_LIGHTSLATE);
                }
                else
                {
                    riv_draw_rect_fill((qc * 16), (15 + NUM_TRACKS * 10) + ((qq - note_row_from) * 8), 16, 8, RIV_COLOR_WHITE);
                }
                if (qc == 0 && notename[0] == 'C' && notename[1] != '#') {
                    riv_draw_text(notename, RIV_SPRITESHEET_FONT_3X5, RIV_CENTER, 8 + (qc * 16), 4 + (15 + NUM_TRACKS * 10) + ((qq - note_row_from) * 8), 1, RIV_COLOR_GREY);
                }
            }
            if ((qq - note_row_from) == selected_note_row && qc == selected_note_col)
            {
                // Current note cursor
                if (notes[blocks[selected_track][selected_block]][qq][qc] == -1000)
                {
                    riv_draw_rect_fill((qc * 16), (15 + NUM_TRACKS * 10) + ((qq - note_row_from) * 8), 16, 8, RIV_COLOR_YELLOW);
                }
            }
            if (notes[blocks[selected_track][selected_block]][qq][qc] >= 1 && notes[blocks[selected_track][selected_block]][qq][qc] <= 8)
            {
                riv_draw_rect_fill((qc * 16), (15 + NUM_TRACKS * 10) + ((qq - note_row_from) * 8), 16 * notes[blocks[selected_track][selected_block]][qq][qc], 8, RIV_COLOR_GREEN);
                riv_draw_rect_line((qc * 16), (15 + NUM_TRACKS * 10) + ((qq - note_row_from) * 8), 16 * notes[blocks[selected_track][selected_block]][qq][qc], 8, RIV_COLOR_BLACK);
                already_outlined = notes[blocks[selected_track][selected_block]][qq][qc] - 1;
            }
            else
            {
                if (already_outlined == 0)
                {
                    riv_draw_line((qc * 16), (15 + NUM_TRACKS * 10) + ((qq - note_row_from) * 8), (qc * 16) + 16, (15 + NUM_TRACKS * 10) + ((qq - note_row_from) * 8), RIV_COLOR_SLATE);
                    riv_draw_line((qc * 16), (15 + NUM_TRACKS * 10) + ((qq - note_row_from) * 8), (qc * 16), (15 + NUM_TRACKS * 10) + ((qq - note_row_from) * 8) + 8, RIV_COLOR_SLATE);
                }
                else
                {
                    already_outlined--;
                }
            }
            if ((qq - note_row_from) == selected_note_row && qc == selected_note_col)
            {
                // Current note outline + text
                riv_draw_text(notename, RIV_SPRITESHEET_FONT_3X5, RIV_CENTER, 8 + (qc * 16), 4 + (15 + NUM_TRACKS * 10) + ((qq - note_row_from) * 8), 1, RIV_COLOR_GREY);
                riv_draw_rect_line((qc * 16) + 1, (15 + NUM_TRACKS * 10) + ((qq - note_row_from) * 8) + 1, 15, 7, RIV_COLOR_YELLOW);
            }
        }
        free(notename);
    }
}

void instrument_mode_loop() {
    if (riv->keys[RIV_KEYCODE_H].down)
    {
        help_mode_instrument_loop();
        return;
    }

    // Check instrument key
    check_instrument_keys();
    // Draw instrument date for tracks
    for (int track = 0; track < NUM_TRACKS; track++)
    {
        char buff[15];
        snprintf(buff, 15, "Track: %d", track);
        int offx = 9;
        int offy = 5;
        riv_draw_text("TYPE:", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1 + offx, track * 32 + 2 + offy, 1, RIV_COLOR_GREEN);
        if (instrument_mode_track == track && instrument_mode_item == 0) {
            riv_draw_rect_fill(33 + offx, track * 32 + 1 + offy, 60 - 4, 9, RIV_COLOR_LIGHTSLATE);
        }
        char *type_text = "PULSE";
        if (instruments[track].type == RIV_WAVEFORM_SINE) {
            type_text = "SINE";
        }
        if (instruments[track].type == RIV_WAVEFORM_SQUARE)
        {
            type_text = "SQUARE";
        }
        if (instruments[track].type == RIV_WAVEFORM_TRIANGLE)
        {
            type_text = "TRIANGLE";
        }
        if (instruments[track].type == RIV_WAVEFORM_SAWTOOTH)
        {
            type_text = "SAWTOOTH";
        }
        if (instruments[track].type == RIV_WAVEFORM_NOISE)
        {
            type_text = "NOISE";
        }
        riv_draw_text(type_text, RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 35 + offx, track * 32 + 2 + offy, 1, RIV_COLOR_WHITE);
        riv_draw_text("AMP", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 95 + offx, track * 32 + 2 + offy, 1, RIV_COLOR_GREEN);
        if (instrument_mode_track == track && instrument_mode_item == 1)
        {
            riv_draw_rect_fill(120 + offx - 3, track * 32 + 2 + offy - 1, 47, 9, RIV_COLOR_LIGHTSLATE);
        }
        snprintf(buff, 15, "%f", instruments[track].amplitude);
        riv_draw_text(buff, RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 117 + offx, track * 32 + 2 + offy, 1, RIV_COLOR_WHITE);
        riv_draw_text("SUS", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 170 + offx, track * 32 + 2 + offy, 1, RIV_COLOR_GREEN);
        if (instrument_mode_track == track && instrument_mode_item == 2)
        {
            riv_draw_rect_fill(195 + offx - 3, track * 32 + 2 + offy - 1, 47, 9, RIV_COLOR_LIGHTSLATE);
        }
        snprintf(buff, 15, "%f", instruments[track].sustain_level);
        riv_draw_text(buff, RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 192 + offx, track * 32 + 2 + offy, 1, RIV_COLOR_WHITE);
        riv_draw_text("A", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 1 + offx, track * 32 + 2 + 12 + offy, 1, RIV_COLOR_GREEN);
        if (instrument_mode_track == track && instrument_mode_item == 3)
        {
            riv_draw_rect_fill(12 + offx - 3, track * 32 + 2 + 12 + offy - 1, 47, 9, RIV_COLOR_LIGHTSLATE);
        }
        snprintf(buff, 15, "%f", instruments[track].attack);
        riv_draw_text(buff, RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 9 + offx, track * 32 + 2 + 12 + offy, 1, RIV_COLOR_WHITE);
        riv_draw_text("D", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 62 + offx, track * 32 + 2 + 12 + offy, 1, RIV_COLOR_GREEN);
        if (instrument_mode_track == track && instrument_mode_item == 4)
        {
            riv_draw_rect_fill(73 + offx - 3, track * 32 + 2 + 12 + offy - 1, 47, 9, RIV_COLOR_LIGHTSLATE);
        }
        snprintf(buff, 15, "%f", instruments[track].decay);
        riv_draw_text(buff, RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 70 + offx, track * 32 + 2 + 12 + offy, 1, RIV_COLOR_WHITE);
        riv_draw_text("R", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 123 + offx, track * 32 + 2 + 12 + offy, 1, RIV_COLOR_GREEN);
        if (instrument_mode_track == track && instrument_mode_item == 5)
        {
            riv_draw_rect_fill(134 + offx - 3, track * 32 + 2 + 12 + offy - 1, 47, 9, RIV_COLOR_LIGHTSLATE);
        }
        snprintf(buff, 15, "%f", instruments[track].release);
        riv_draw_text(buff, RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 131 + offx, track * 32 + 2 + 12 + offy, 1, RIV_COLOR_WHITE);
        riv_draw_text("C", RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 184 + offx, track * 32 + 2 + 12 + offy, 1, RIV_COLOR_GREEN);
        if (instrument_mode_track == track && instrument_mode_item == 6)
        {
            riv_draw_rect_fill(195 + offx - 3, track * 32 + 2 + 12 + offy - 1, 47, 9, RIV_COLOR_LIGHTSLATE);
        }
        snprintf(buff, 15, "%f", instruments[track].duty_cycle);
        riv_draw_text(buff, RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, 192 + offx, track * 32 + 2 + 12 + offy, 1, RIV_COLOR_WHITE);
        riv_draw_line(0, track * 32 + 32, 256, track * 32 + 32, RIV_COLOR_LIGHTSLATE);
    }
}

void file_mode_loop()
{
    if (riv->keys[RIV_KEYCODE_H].down) {
        help_mode_file_loop();
        return;
    }

    check_file_keys();

    for (int x = -1; x < max_file; x++)
    {
        char fn[23];
        int col = RIV_COLOR_WHITE;
        if (x == -1)
        {
            snprintf(fn, 23, "%s", "NEW COMPOSITION");
            col = RIV_COLOR_GREEN;
        }
        else
        {
            snprintf(fn, 23, "%s", files[x]);
        }
        int offset = 1 + (int)floor((x + 1) / 25) * 125;
        if (x == highlighted_file)
        {
            riv_draw_rect_fill(offset - 1, ((x + 1) % 25) * 10, offset + 123, 10, RIV_COLOR_SLATE);
        }
        riv_draw_text(fn, RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, offset, 1 + ((x + 1) % 25) * 10, 1, col);
    }
}

void play_music() {

    if (music_playing == true)
    {
        // If music is playing then draw an indicator of that at the top
        int range = (stop_playing + 1) - start_playing;                             // Range is the number of blocks to be played
        int range_beats = range * 16;                                               // The number of beats (quarter notes) over this range
        unsigned long frame_offset = riv->frame - music_start_frame;                // An offset as an indicator of time
        float seconds_passed = (float)frame_offset / (float)riv->target_fps;        // Seconds passed
        seconds_passed = seconds_passed * 2.0;                                      // Each of our notes is actually an eight note
        int beats_passed = (int)floor(seconds_passed * ((float)target_bpm / 60.0)); // The number of beats that have passed
        int beat_in_range = beats_passed % range_beats;                             // The current beat in the range being played
        int line_x = beat_in_range + (start_playing * 16);
        if (screenmode == MUSIC) {
            riv_draw_line(line_x, 2, line_x, (NUM_TRACKS * 10), RIV_COLOR_ORANGE);
        }
        // If the play pointer is over the current editted block then also
        // draw an indicator in the note area
        if (line_x >= (selected_block * 16) && line_x <= (selected_block * 16) + 15 && screenmode == MUSIC)
        {
            int line_x_b = line_x % 16;
            riv_draw_line(line_x_b * 16, (NUM_TRACKS * 10) + 14, line_x_b * 16, (NUM_TRACKS * 10) + 14 + ((note_row_to - note_row_from) * 8), RIV_COLOR_ORANGE);
        }

        // See if we need to play a note. To do this we need to know the current block column being played
        // and the current note column
        int current_playing_block_column = (int)floor(line_x / 16);
        int current_playing_note_column = line_x - (current_playing_block_column * 16);
        for (int p = 0; p < NUM_TRACKS; p++)
        {
            int this_block_id = blocks[p][current_playing_block_column];
            for (int qq = 0; qq < 128; qq++)
            {
                int thisnote = notes[this_block_id][qq][current_playing_note_column];
                if (thisnote > -1 && note_playing[this_block_id][qq][current_playing_note_column] == false)
                {
                    // There's a note.
                    // Copy a waveform to the waveform for this track and note
                    waveforms[p][qq].type = instruments[p].type;
                    waveforms[p][qq].attack = instruments[p].attack;
                    waveforms[p][qq].decay = instruments[p].decay;
                    waveforms[p][qq].sustain = 0;
                    waveforms[p][qq].release = instruments[p].release;
                    waveforms[p][qq].amplitude = instruments[p].amplitude;
                    waveforms[p][qq].sustain_level = instruments[p].sustain_level;
                    waveforms[p][qq].duty_cycle = instruments[p].duty_cycle;
                    // Calculate the frequency
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
                    note_playing[this_block_id][qq][current_playing_note_column] = true;
                }
            }
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

void main_loop()
{
    riv_clear(0);

    if (screenmode == MUSIC) {
        music_mode_loop();
    }

    if (screenmode == INSTRUMENT) {
        instrument_mode_loop();
    }

    if (screenmode == FILE_OPEN) {
        file_mode_loop();
    }

    play_music();
}

// Entry point
int main()
{
    check_rivemu_workspace();
    init();

    do
    {
        main_loop();
    } while (riv_present());
    return 0;
}
