// Studio Rivalz — Play Mode timing judge (Perfect / Great / Good / Miss).
#pragma once
#include "game/chart.h"

#define WINDOW_PERFECT 0.045f
#define WINDOW_GREAT   0.090f
#define WINDOW_GOOD    0.140f
#define MAX_CUES 64

typedef enum { J_NONE = 0, J_PERFECT, J_GREAT, J_GOOD, J_MISS, J_COUNT } Judgement;

typedef struct {
    float time;        // seconds into the song
    CueButton button;
    int step_index;    // index into Routine.steps
    Judgement result;  // J_NONE until judged
    float error;       // press time - cue time (seconds)
} Cue;

typedef struct {
    Cue cues[MAX_CUES];
    int n;
    int combo, max_combo;
    int score;
    int counts[J_COUNT];
} CueTrack;

void cuetrack_build(CueTrack *t, const Routine *r, Difficulty d);
// A button was pressed at song time `sec`. Returns the judgement (J_NONE if
// no cue was close enough: a stray press). *cue_out gets the cue index or -1.
Judgement cuetrack_press(CueTrack *t, CueButton b, float sec, int *cue_out);
// Mark cues whose window has fully passed as misses. Returns how many.
int cuetrack_update(CueTrack *t, float sec);
// Is the step at `step_index` a cue that was missed?
bool cuetrack_step_missed(const CueTrack *t, int step_index);
// 0..1 — points earned / max points.
float cuetrack_accuracy(const CueTrack *t);
const char *judgement_name(Judgement j);
int judgement_points(Judgement j);
