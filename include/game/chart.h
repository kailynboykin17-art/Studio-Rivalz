// Studio Rivalz — songs, choreography and beat-synced poses.
// Pure C (no libctru) so it can be unit-tested on a normal computer.
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MV_ENTRANCE = 0, // walk on from stage left
    MV_READY,        // hold prep pose before the routine
    MV_ARMS,         // arm line (auto groove, never a button cue)
    MV_STEP,         // B — step / ball change
    MV_KICK,         // A — high kick
    MV_TURN,         // X — pirouette (2 beats)
    MV_LEAP,         // Y — grand jeté (2 beats)
    MV_FINISH,       // A — final hit + held pose
    MV_COUNT
} MoveType;

typedef enum { CUE_NONE = -1, CUE_A = 0, CUE_B, CUE_X, CUE_Y, CUE_COUNT } CueButton;

typedef enum { DIFF_EASY = 0, DIFF_NORMAL, DIFF_HARD, DIFF_COUNT } Difficulty;

// Fixed frame order of every dancer sprite sheet (see docs/ART_SPEC.md).
typedef enum {
    FR_IDLE = 0, FR_PREP, FR_ARMS, FR_STEP, FR_KICK, FR_LEAP,
    FR_TURN_A, FR_TURN_B, FR_LAND, FR_FINISH, FR_CELEBRATE, FR_DISAPPOINT,
    FR_COUNT
} DancerFrame;

typedef struct {
    float beat;   // start beat (beat 0 = first downbeat of the audio file)
    uint8_t move; // MoveType
    float beats;  // duration in beats
} Step;

typedef struct {
    const char *id;
    const char *title;
    const char *style;
    float bpm;
    float first_beat_sec; // time of beat 0 inside the file
    int total_beats;
    int count_in_beats;   // "5-6-7-8" intro beats before the routine
    float duration_sec;
    const char *path;     // romfs path
} SongInfo;

typedef struct {
    const char *name;
    const SongInfo *song;
    const Step *steps;
    int num_steps;
} Routine;

typedef struct {
    DancerFrame frame;
    float x;    // horizontal offset from stage centre, pixels
    float lift; // pixels above the floor (leaps)
    bool flip;  // mirror horizontally
} Pose;

const SongInfo *song_spotlight_sass(void);
const Routine *routine_jazz_solo(void);

const char *move_name(MoveType m);
CueButton move_button(MoveType m);
// Lowest difficulty at which this move becomes a button cue (-1 = never cued).
int move_cue_difficulty(MoveType m);
const char *difficulty_name(Difficulty d);
const char *cue_button_name(CueButton b);

float song_beat_to_sec(const SongInfo *s, float beat);
float song_sec_to_beat(const SongInfo *s, float sec);

// Index of the step active at `beat`, or -1.
int routine_step_at(const Routine *r, float beat);
// Pose of the dancer at `beat`, following the choreography exactly.
Pose routine_pose(const Routine *r, float beat);
// Which 8-count we are in (1-based) and the count inside it (1..8). 0 during count-in.
void routine_count(const Routine *r, float beat, int *eight, int *count);
