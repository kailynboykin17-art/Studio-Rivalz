// Studio Rivalz — studio sim state, weekly actions, judging and save format.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "game/judge.h"

// ---------------------------------------------------------------- dancers
typedef struct {
    int32_t technique, flexibility, performance, musicality, stamina; // 0..100
} Stats;

typedef struct {
    int32_t roster_id;  // 1..25 (see roster.h)
    char name[24];
    Stats stats;
    int32_t energy;     // 0..100 — tired dancers lose points
    int32_t rehearsal;  // 0..5 — how well the routine is rehearsed
} Dancer;

// ---------------------------------------------------------------- studio
#define NUM_RIVALS 5
#define ENTRY_FEE 100

typedef enum { MODE_PLAY = 0, MODE_WATCH = 1 } PerformMode;

typedef struct {
    int32_t week, funds, trophies, reputation;
    int32_t comps_entered, wins;
    int32_t best_total_tenths;   // best judged score, x10
    int32_t timing_offset_ms;    // Play Mode latency calibration
    int32_t difficulty, mode;    // last used
    int32_t rival_growth[NUM_RIVALS]; // tenths of a point each rival has improved
    Dancer dancer;               // the one playable dancer in this slice
} Studio;

void studio_new(Studio *s);

typedef enum { ACT_TRAIN = 0, ACT_REHEARSE, ACT_REST, ACT_COUNT } StudioAction;
#define TRAIN_COST 75

const char *action_name(StudioAction a);
// Returns NULL if allowed, else a short reason.
const char *studio_cannot(const Studio *s, StudioAction a);
const char *studio_cannot_compete(const Studio *s);
// Applies the action, advances one week, writes a one-line summary.
void studio_apply(Studio *s, StudioAction a, char *msg, size_t msglen);

// ---------------------------------------------------------------- judging
enum { CAT_TECHNIQUE = 0, CAT_MUSICALITY, CAT_CHOREO, CAT_PERFORMANCE, CAT_PRESENTATION, CAT_COUNT };
extern const int CAT_WEIGHT[CAT_COUNT]; // percentages, sum 100
const char *cat_name(int c);

#define NUM_JUDGES 3
#define MAX_ENTRIES (NUM_RIVALS + 1)

typedef struct {
    char name[24];
    char style[16];
    bool is_player;
    float cat[CAT_COUNT];            // 0..100 per category (before judge variation)
    int32_t judge_tenths[NUM_JUDGES];
    int32_t total_tenths;            // mean of judges, x10
    int32_t place;                   // 1 = first
    bool tied;
} Entry;

typedef struct {
    Entry e[MAX_ENTRIES];
    int n;
    int order[MAX_ENTRIES]; // indexes into e[], best first
    int player;             // index of player entry or -1
    uint32_t seed;
} Results;

const char *rival_name(int i);
const char *rival_style(int i);

// Category scores for the player's routine. `track` is required for Play
// Mode (player skill) and ignored for Watch Mode (dancer stats only).
void studio_player_categories(const Studio *s, PerformMode mode, Difficulty d,
                              const CueTrack *track, float out[CAT_COUNT]);
void studio_rival_categories(const Studio *s, int rival, float out[CAT_COUNT]);

// Build the 6-entry event, apply 3 judges, rank with explicit tiebreaks:
// total -> Technique -> Performance -> shared placement.
void compete_run(Results *r, const Studio *s, const float player_cat[CAT_COUNT], uint32_t seed);

typedef struct { int32_t place, money, rep; bool trophy; bool tied; } Reward;
Reward reward_for_place(int place);
// Pays out, grows rivals, uses energy, advances the week.
Reward studio_apply_results(Studio *s, const Results *r);

// ---------------------------------------------------------------- saves
#define SAVE_MAGIC 0x5A565253u /* "SRVZ" */
#define SAVE_VERSION 1
typedef struct {
    uint32_t magic, version, size;
    Studio studio;
    uint32_t crc;
} SaveBlob;

uint32_t crc32_bytes(const void *data, size_t len);
void save_pack(const Studio *s, SaveBlob *out);
// 0 ok, -1 bad magic, -2 wrong version/size, -3 checksum mismatch
int save_unpack(const SaveBlob *in, Studio *s);
