#include "game/chart.h"
#include <math.h>
#include <stddef.h>

// ---------------------------------------------------------------------------
// Song: "Spotlight Sass" — original prototype instrumental made by
// tools/gen_music.py. 124 BPM, 62 beats = 30.0 s, beat 0 at sample 0.
// ---------------------------------------------------------------------------
static const SongInfo SPOTLIGHT_SASS = {
    .id = "spotlight_sass",
    .title = "Spotlight Sass",
    .style = "Jazz",
    .bpm = 124.0f,
    .first_beat_sec = 0.0f,
    .total_beats = 62,
    .count_in_beats = 8,
    .duration_sec = 30.0f,
    .path = "romfs:/music/spotlight_sass_30.wav",
};

const SongInfo *song_spotlight_sass(void) { return &SPOTLIGHT_SASS; }

// ---------------------------------------------------------------------------
// 30-second jazz solo. Beats 0-7 count-in/entrance, six 8-counts (8-55),
// finish hit on 56 held to the end of the music.
// ---------------------------------------------------------------------------
#define S(b, m, d) { (float)(b), (uint8_t)(m), (float)(d) }
static const Step JAZZ_SOLO[] = {
    S(0, MV_ENTRANCE, 6), S(6, MV_READY, 2),
    // 8-count 1
    S(8, MV_ARMS, 1), S(9, MV_ARMS, 1), S(10, MV_STEP, 1), S(11, MV_STEP, 1),
    S(12, MV_KICK, 1), S(13, MV_STEP, 1), S(14, MV_KICK, 1), S(15, MV_ARMS, 1),
    // 8-count 2
    S(16, MV_TURN, 2), S(18, MV_STEP, 1), S(19, MV_STEP, 1),
    S(20, MV_LEAP, 2), S(22, MV_ARMS, 1), S(23, MV_KICK, 1),
    // 8-count 3
    S(24, MV_KICK, 1), S(25, MV_KICK, 1), S(26, MV_TURN, 2),
    S(28, MV_STEP, 1), S(29, MV_STEP, 1), S(30, MV_LEAP, 2),
    // 8-count 4
    S(32, MV_ARMS, 1), S(33, MV_STEP, 1), S(34, MV_KICK, 1), S(35, MV_STEP, 1),
    S(36, MV_TURN, 2), S(38, MV_KICK, 1), S(39, MV_ARMS, 1),
    // 8-count 5
    S(40, MV_LEAP, 2), S(42, MV_STEP, 1), S(43, MV_KICK, 1),
    S(44, MV_TURN, 2), S(46, MV_LEAP, 2),
    // 8-count 6
    S(48, MV_STEP, 1), S(49, MV_KICK, 1), S(50, MV_STEP, 1), S(51, MV_KICK, 1),
    S(52, MV_TURN, 2), S(54, MV_LEAP, 2),
    // ending
    S(56, MV_FINISH, 6),
};
#undef S

static const Routine JAZZ_ROUTINE = {
    .name = "Spotlight Sass — Jazz Solo",
    .song = &SPOTLIGHT_SASS,
    .steps = JAZZ_SOLO,
    .num_steps = (int)(sizeof(JAZZ_SOLO) / sizeof(JAZZ_SOLO[0])),
};

const Routine *routine_jazz_solo(void) { return &JAZZ_ROUTINE; }

// ---------------------------------------------------------------------------
const char *move_name(MoveType m) {
    static const char *names[MV_COUNT] = {
        "Entrance", "Ready", "Arm line", "Step", "Kick", "Turn", "Leap", "Finish"};
    return (m >= 0 && m < MV_COUNT) ? names[m] : "?";
}

CueButton move_button(MoveType m) {
    switch (m) {
    case MV_KICK: return CUE_A;
    case MV_STEP: return CUE_B;
    case MV_TURN: return CUE_X;
    case MV_LEAP: return CUE_Y;
    case MV_FINISH: return CUE_A;
    default: return CUE_NONE;
    }
}

int move_cue_difficulty(MoveType m) {
    switch (m) {
    case MV_TURN:
    case MV_LEAP:
    case MV_FINISH: return DIFF_EASY;
    case MV_KICK: return DIFF_NORMAL;
    case MV_STEP: return DIFF_HARD;
    default: return -1;
    }
}

const char *difficulty_name(Difficulty d) {
    static const char *n[DIFF_COUNT] = {"Easy", "Normal", "Hard"};
    return (d >= 0 && d < DIFF_COUNT) ? n[d] : "?";
}

const char *cue_button_name(CueButton b) {
    static const char *n[CUE_COUNT] = {"A", "B", "X", "Y"};
    return (b >= 0 && b < CUE_COUNT) ? n[b] : "-";
}

float song_beat_to_sec(const SongInfo *s, float beat) {
    return s->first_beat_sec + beat * 60.0f / s->bpm;
}

float song_sec_to_beat(const SongInfo *s, float sec) {
    return (sec - s->first_beat_sec) * s->bpm / 60.0f;
}

int routine_step_at(const Routine *r, float beat) {
    for (int i = r->num_steps - 1; i >= 0; i--) {
        const Step *st = &r->steps[i];
        if (beat >= st->beat) return (beat < st->beat + st->beats || i == r->num_steps - 1) ? i : -1;
    }
    return -1;
}

void routine_count(const Routine *r, float beat, int *eight, int *count) {
    int cin = r->song->count_in_beats;
    if (beat < cin) { *eight = 0; *count = (beat < 0) ? 0 : (int)floorf(beat) + 1; return; }
    int b = (int)floorf(beat) - cin;
    *eight = b / 8 + 1;
    *count = b % 8 + 1;
}

// ---------------------------------------------------------------------------
// Stage travel. Every step's displacement is fixed by the choreography so
// Play Mode and Watch Mode put the dancer in exactly the same places.
// ---------------------------------------------------------------------------
#define ENTRANCE_START_X (-150.0f)
#define STAGE_LIMIT_X (110.0f)

static float step_travel(const Routine *r, int idx, float x_before) {
    MoveType m = (MoveType)r->steps[idx].move;
    if (m == MV_STEP) {
        int n = 0; // alternate direction per step
        for (int i = 0; i < idx; i++) if (r->steps[i].move == MV_STEP) n++;
        return (n % 2 == 0) ? 12.0f : -12.0f;
    }
    if (m == MV_LEAP) return (x_before > 0.0f) ? -40.0f : 40.0f; // leap toward centre
    if (m == MV_ENTRANCE) return -ENTRANCE_START_X;
    return 0.0f;
}

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static float smooth(float t) { return t * t * (3.0f - 2.0f * t); }

static DancerFrame frame_for(MoveType m, float t, int step_parity, bool *flip) {
    *flip = false;
    switch (m) {
    case MV_ENTRANCE: {
        int half = (int)floorf(t * 12.0f); // 6 beats -> 12 half-beat footfalls
        return (half % 2 == 0) ? FR_STEP : FR_IDLE;
    }
    case MV_READY: return FR_PREP;
    case MV_ARMS: return t < 0.3f ? FR_IDLE : FR_ARMS;
    case MV_STEP:
        *flip = step_parity;
        return t < 0.25f ? FR_PREP : (t < 0.75f ? FR_STEP : FR_IDLE);
    case MV_KICK: return t < 0.2f ? FR_PREP : (t < 0.7f ? FR_KICK : FR_STEP);
    case MV_TURN:
        if (t < 0.15f) return FR_PREP;
        if (t < 0.40f) return FR_TURN_A;
        if (t < 0.60f) return FR_TURN_B;
        if (t < 0.85f) { *flip = true; return FR_TURN_A; }
        return FR_IDLE;
    case MV_LEAP: return t < 0.2f ? FR_PREP : (t < 0.65f ? FR_LEAP : FR_LAND);
    case MV_FINISH: return t < 0.05f ? FR_PREP : FR_FINISH;
    default: return FR_IDLE;
    }
}

Pose routine_pose(const Routine *r, float beat) {
    Pose p = {FR_IDLE, ENTRANCE_START_X, 0.0f, false};
    if (beat < 0.0f) return p;
    float x = ENTRANCE_START_X;
    int steps_seen = 0;
    for (int i = 0; i < r->num_steps; i++) {
        const Step *st = &r->steps[i];
        if (beat < st->beat) break;
        float t = (beat - st->beat) / st->beats;
        float travel = clampf(x + step_travel(r, i, x), -STAGE_LIMIT_X, STAGE_LIMIT_X) - x;
        bool active = t < 1.0f || i == r->num_steps - 1;
        if (t > 1.0f) t = 1.0f;
        MoveType m = (MoveType)st->move;
        if (active) {
            // travel happens during the main part of the move
            float tt = t;
            if (m == MV_LEAP) tt = clampf((t - 0.2f) / 0.45f, 0.0f, 1.0f);
            else if (m == MV_STEP) tt = clampf((t - 0.25f) / 0.5f, 0.0f, 1.0f);
            p.x = x + travel * (m == MV_ENTRANCE ? t : smooth(tt));
            p.frame = frame_for(m, t, steps_seen % 2, &p.flip);
            p.lift = 0.0f;
            if (m == MV_LEAP && t >= 0.2f && t < 0.65f) p.lift = 22.0f * sinf((t - 0.2f) / 0.45f * 3.14159265f);
        }
        x += travel;
        if (m == MV_STEP) steps_seen++;
        if (!active) { p.x = x; p.frame = FR_IDLE; p.flip = false; p.lift = 0.0f; }
    }
    return p;
}
