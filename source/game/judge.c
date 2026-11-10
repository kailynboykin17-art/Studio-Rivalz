#include "game/judge.h"
#include <math.h>
#include <string.h>

void cuetrack_build(CueTrack *t, const Routine *r, Difficulty d) {
    memset(t, 0, sizeof(*t));
    for (int i = 0; i < r->num_steps && t->n < MAX_CUES; i++) {
        MoveType m = (MoveType)r->steps[i].move;
        int need = move_cue_difficulty(m);
        if (need < 0 || (int)d < need) continue;
        Cue *c = &t->cues[t->n++];
        c->time = song_beat_to_sec(r->song, r->steps[i].beat);
        c->button = move_button(m);
        c->step_index = i;
        c->result = J_NONE;
        c->error = 0.0f;
    }
}

int judgement_points(Judgement j) {
    switch (j) {
    case J_PERFECT: return 100;
    case J_GREAT: return 70;
    case J_GOOD: return 40;
    default: return 0;
    }
}

const char *judgement_name(Judgement j) {
    static const char *n[J_COUNT] = {"", "PERFECT", "GREAT", "GOOD", "MISS"};
    return (j >= 0 && j < J_COUNT) ? n[j] : "";
}

static void record(CueTrack *t, Cue *c, Judgement j, float err) {
    c->result = j;
    c->error = err;
    t->counts[j]++;
    if (j == J_MISS) {
        t->combo = 0;
    } else {
        t->combo++;
        if (t->combo > t->max_combo) t->max_combo = t->combo;
        int bonus = t->combo > 20 ? 20 : t->combo;
        t->score += judgement_points(j) + bonus * 5;
    }
}

Judgement cuetrack_press(CueTrack *t, CueButton b, float sec, int *cue_out) {
    int best = -1;
    float best_abs = 1e9f;
    for (int i = 0; i < t->n; i++) {
        Cue *c = &t->cues[i];
        if (c->result != J_NONE) continue;
        float a = fabsf(sec - c->time);
        if (a <= WINDOW_GOOD && a < best_abs) { best = i; best_abs = a; }
    }
    if (cue_out) *cue_out = best;
    if (best < 0) return J_NONE;
    Cue *c = &t->cues[best];
    float err = sec - c->time;
    Judgement j;
    if (b != c->button) j = J_MISS; // wrong move on the cue
    else if (best_abs <= WINDOW_PERFECT) j = J_PERFECT;
    else if (best_abs <= WINDOW_GREAT) j = J_GREAT;
    else j = J_GOOD;
    record(t, c, j, err);
    return j;
}

int cuetrack_update(CueTrack *t, float sec) {
    int missed = 0;
    for (int i = 0; i < t->n; i++) {
        Cue *c = &t->cues[i];
        if (c->result == J_NONE && sec > c->time + WINDOW_GOOD) {
            record(t, c, J_MISS, sec - c->time);
            missed++;
        }
    }
    return missed;
}

bool cuetrack_step_missed(const CueTrack *t, int step_index) {
    for (int i = 0; i < t->n; i++)
        if (t->cues[i].step_index == step_index) return t->cues[i].result == J_MISS;
    return false;
}

float cuetrack_accuracy(const CueTrack *t) {
    if (t->n == 0) return 0.0f;
    int pts = t->counts[J_PERFECT] * 100 + t->counts[J_GREAT] * 70 + t->counts[J_GOOD] * 40;
    return (float)pts / (float)(t->n * 100);
}
