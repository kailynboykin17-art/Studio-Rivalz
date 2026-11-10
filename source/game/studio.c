#include "game/studio.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// ============================================================ studio basics
void studio_new(Studio *s) {
    memset(s, 0, sizeof(*s));
    s->week = 1;
    s->funds = 1000;
    s->difficulty = DIFF_NORMAL;
    s->mode = MODE_PLAY;
    Dancer *d = &s->dancer;
    d->roster_id = 1;
    snprintf(d->name, sizeof(d->name), "%s", "Avery Lane");
    d->stats = (Stats){.technique = 55, .flexibility = 60, .performance = 58, .musicality = 52, .stamina = 60};
    d->energy = 80;
    d->rehearsal = 1;
}

static int32_t clampi(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : (v > hi ? hi : v); }
static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

const char *action_name(StudioAction a) {
    static const char *n[ACT_COUNT] = {"Technique Class", "Rehearse Routine", "Rest Day"};
    return (a >= 0 && a < ACT_COUNT) ? n[a] : "?";
}

const char *studio_cannot(const Studio *s, StudioAction a) {
    switch (a) {
    case ACT_TRAIN:
        if (s->funds < TRAIN_COST) return "Not enough money ($75)";
        if (s->dancer.energy < 20) return "Too tired - rest first";
        return NULL;
    case ACT_REHEARSE:
        if (s->dancer.rehearsal >= 5) return "Routine fully rehearsed";
        if (s->dancer.energy < 15) return "Too tired - rest first";
        return NULL;
    case ACT_REST:
        if (s->dancer.energy >= 100) return "Already fully rested";
        return NULL;
    default: return "Unknown action";
    }
}

const char *studio_cannot_compete(const Studio *s) {
    if (s->funds < ENTRY_FEE) return "Entry fee is $100";
    if (s->dancer.energy < 25) return "Too tired to compete";
    return NULL;
}

void studio_apply(Studio *s, StudioAction a, char *msg, size_t msglen) {
    Dancer *d = &s->dancer;
    const char *why = studio_cannot(s, a);
    if (why) { snprintf(msg, msglen, "%s", why); return; }
    switch (a) {
    case ACT_TRAIN:
        s->funds -= TRAIN_COST;
        d->stats.technique = clampi(d->stats.technique + 3, 0, 100);
        d->stats.flexibility = clampi(d->stats.flexibility + 1, 0, 100);
        d->energy = clampi(d->energy - 20, 0, 100);
        snprintf(msg, msglen, "Class done: +3 Technique, +1 Flexibility");
        break;
    case ACT_REHEARSE:
        d->rehearsal = clampi(d->rehearsal + 1, 0, 5);
        d->stats.musicality = clampi(d->stats.musicality + 1, 0, 100);
        d->energy = clampi(d->energy - 15, 0, 100);
        snprintf(msg, msglen, "Rehearsal %d/5, +1 Musicality", (int)d->rehearsal);
        break;
    case ACT_REST:
        d->energy = clampi(d->energy + 40, 0, 100);
        snprintf(msg, msglen, "Rested: energy now %d", (int)d->energy);
        break;
    default: break;
    }
    s->week++;
}

// ============================================================ judging
const int CAT_WEIGHT[CAT_COUNT] = {30, 20, 20, 20, 10};

const char *cat_name(int c) {
    static const char *n[CAT_COUNT] = {"Technique", "Musicality", "Choreography", "Performance", "Presentation"};
    return (c >= 0 && c < CAT_COUNT) ? n[c] : "?";
}

typedef struct { const char *name, *style; float base[CAT_COUNT]; } RivalDef;
static const RivalDef RIVALS[NUM_RIVALS] = {
    {"Velvet Steps", "Ballet", {66, 60, 58, 54, 66}},
    {"Neon Pulse", "Jazz", {60, 62, 60, 64, 64}},
    {"Moonlit Motion", "Contemporary", {68, 66, 66, 62, 68}},
    {"Royal Rhythm", "Hip-Hop", {64, 72, 68, 72, 70}},
    {"Silver Beat", "Jazz Funk", {74, 72, 72, 74, 74}},
};

const char *rival_name(int i) { return (i >= 0 && i < NUM_RIVALS) ? RIVALS[i].name : "?"; }
const char *rival_style(int i) { return (i >= 0 && i < NUM_RIVALS) ? RIVALS[i].style : "?"; }

void studio_rival_categories(const Studio *s, int rival, float out[CAT_COUNT]) {
    for (int c = 0; c < CAT_COUNT; c++)
        out[c] = clampf(RIVALS[rival].base[c] + s->rival_growth[rival] / 10.0f, 0, 100);
}

void studio_player_categories(const Studio *s, PerformMode mode, Difficulty d,
                              const CueTrack *track, float out[CAT_COUNT]) {
    const Dancer *dn = &s->dancer;
    const Stats *st = &dn->stats;
    float fatigue = dn->energy < 40 ? (40 - dn->energy) * 0.25f : 0.0f;
    float reh = (float)dn->rehearsal;

    if (mode == MODE_PLAY && track && track->n > 0) {
        float acc = cuetrack_accuracy(track);
        float combo = (float)track->max_combo / (float)track->n;
        static const float choreo_bonus[DIFF_COUNT] = {0, 6, 12};
        out[CAT_TECHNIQUE] = 0.6f * st->technique + 0.2f * st->flexibility + 20.0f * acc - fatigue;
        out[CAT_MUSICALITY] = 35.0f + 65.0f * acc;
        out[CAT_CHOREO] = 58.0f + reh * 3.0f + choreo_bonus[d];
        out[CAT_PERFORMANCE] = 0.5f * st->performance + 0.1f * st->stamina + 40.0f * combo - fatigue;
    } else {
        static const float choreo_bonus[DIFF_COUNT] = {0, 4, 8};
        float risk = (d == DIFF_HARD && st->technique < 70) ? 4.0f : 0.0f;
        out[CAT_TECHNIQUE] = 0.7f * st->technique + 0.3f * st->flexibility + reh * 1.5f - fatigue - risk;
        out[CAT_MUSICALITY] = st->musicality + reh * 3.0f;
        out[CAT_CHOREO] = 58.0f + reh * 3.0f + choreo_bonus[d];
        out[CAT_PERFORMANCE] = 0.8f * st->performance + 0.2f * st->stamina + reh - fatigue;
    }
    out[CAT_PRESENTATION] = 70.0f; // one costume in this slice
    for (int c = 0; c < CAT_COUNT; c++) out[c] = clampf(out[c], 0, 100);
}

// Deterministic RNG (xorshift32) so a given seed always produces the same event.
static uint32_t rng_next(uint32_t *st) {
    uint32_t x = *st ? *st : 0x9E3779B9u;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return *st = x;
}
static float rng_range(uint32_t *st, float lo, float hi) {
    return lo + (hi - lo) * (float)(rng_next(st) & 0xFFFFFF) / (float)0xFFFFFF;
}

// Each judge leans slightly toward one category.
static const float JUDGE_BIAS[NUM_JUDGES][CAT_COUNT] = {
    {+2, 0, 0, -1, 0},  // technique judge
    {0, +2, 0, 0, -1},  // music judge
    {-1, 0, 0, +2, 0},  // performance judge
};

static int32_t tenths(float v) { return (int32_t)lroundf(v * 10.0f); }

static int entry_cmp(const Entry *a, const Entry *b) {
    if (a->total_tenths != b->total_tenths) return b->total_tenths - a->total_tenths;
    int ta = tenths(a->cat[CAT_TECHNIQUE]), tb = tenths(b->cat[CAT_TECHNIQUE]);
    if (ta != tb) return tb - ta;
    int pa = tenths(a->cat[CAT_PERFORMANCE]), pb = tenths(b->cat[CAT_PERFORMANCE]);
    return pb - pa;
}

void compete_run(Results *r, const Studio *s, const float player_cat[CAT_COUNT], uint32_t seed) {
    memset(r, 0, sizeof(*r));
    r->seed = seed;
    uint32_t rng = seed;
    r->n = MAX_ENTRIES;
    for (int i = 0; i < MAX_ENTRIES; i++) {
        Entry *e = &r->e[i];
        if (i == 0) {
            snprintf(e->name, sizeof(e->name), "%s", s->dancer.name);
            snprintf(e->style, sizeof(e->style), "Jazz");
            e->is_player = true;
            memcpy(e->cat, player_cat, sizeof(e->cat));
        } else {
            snprintf(e->name, sizeof(e->name), "%s", rival_name(i - 1));
            snprintf(e->style, sizeof(e->style), "%s", rival_style(i - 1));
            studio_rival_categories(s, i - 1, e->cat);
            // rivals have good and bad days too
            for (int c = 0; c < CAT_COUNT; c++) e->cat[c] = clampf(e->cat[c] + rng_range(&rng, -2.0f, 2.0f), 0, 100);
        }
        int32_t sum = 0;
        for (int j = 0; j < NUM_JUDGES; j++) {
            float sc = 0;
            for (int c = 0; c < CAT_COUNT; c++) sc += clampf(e->cat[c] + JUDGE_BIAS[j][c], 0, 100) * CAT_WEIGHT[c] / 100.0f;
            sc = clampf(sc + rng_range(&rng, -1.5f, 1.5f), 0, 100);
            e->judge_tenths[j] = tenths(sc);
            sum += e->judge_tenths[j];
        }
        e->total_tenths = (int32_t)lroundf(sum / (float)NUM_JUDGES);
        r->order[i] = i;
    }
    r->player = 0;
    // insertion sort, best first
    for (int i = 1; i < r->n; i++) {
        int k = r->order[i], j = i - 1;
        while (j >= 0 && entry_cmp(&r->e[r->order[j]], &r->e[k]) > 0) { r->order[j + 1] = r->order[j]; j--; }
        r->order[j + 1] = k;
    }
    for (int i = 0; i < r->n; i++) {
        Entry *e = &r->e[r->order[i]];
        if (i > 0 && entry_cmp(&r->e[r->order[i - 1]], e) == 0) {
            Entry *prev = &r->e[r->order[i - 1]];
            e->place = prev->place;
            e->tied = prev->tied = true;
        } else {
            e->place = i + 1;
        }
    }
}

Reward reward_for_place(int place) {
    Reward w = {place, 50, 1, false, false};
    if (place == 1) { w.money = 500; w.rep = 10; w.trophy = true; }
    else if (place == 2) { w.money = 300; w.rep = 6; }
    else if (place == 3) { w.money = 200; w.rep = 4; }
    return w;
}

Reward studio_apply_results(Studio *s, const Results *r) {
    const Entry *pe = &r->e[r->player];
    Reward w = reward_for_place(pe->place);
    w.tied = pe->tied;
    s->funds += w.money - ENTRY_FEE;
    s->reputation += w.rep;
    if (w.trophy) { s->trophies++; s->wins++; }
    if (pe->total_tenths > s->best_total_tenths) s->best_total_tenths = pe->total_tenths;
    s->comps_entered++;
    s->dancer.energy = clampi(s->dancer.energy - 30, 0, 100);
    for (int i = 0; i < NUM_RIVALS; i++) s->rival_growth[i] += 3 + i; // rivals keep improving
    s->week++;
    return w;
}

// ============================================================ saves
uint32_t crc32_bytes(const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= p[i];
        for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

void save_pack(const Studio *s, SaveBlob *out) {
    memset(out, 0, sizeof(*out));
    out->magic = SAVE_MAGIC;
    out->version = SAVE_VERSION;
    out->size = (uint32_t)sizeof(SaveBlob);
    out->studio = *s;
    out->crc = crc32_bytes(out, offsetof(SaveBlob, crc));
}

int save_unpack(const SaveBlob *in, Studio *s) {
    if (in->magic != SAVE_MAGIC) return -1;
    if (in->version != SAVE_VERSION || in->size != sizeof(SaveBlob)) return -2;
    if (crc32_bytes(in, offsetof(SaveBlob, crc)) != in->crc) return -3;
    *s = in->studio;
    return 0;
}
