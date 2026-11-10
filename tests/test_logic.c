// Host unit tests for Studio Rivalz game logic: `make -C tests run`
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "game/chart.h"
#include "game/judge.h"
#include "game/roster.h"
#include "game/studio.h"
#include "game/wav.h"

static int failures = 0, checks = 0;
#define CHECK(cond) do { checks++; if (!(cond)) { failures++; printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

static void test_song_timing(void) {
    const SongInfo *s = song_spotlight_sass();
    float end = song_beat_to_sec(s, (float)s->total_beats);
    CHECK(fabsf(end - 30.0f) < 0.02f);                 // 62 beats @124 = 30.0 s
    CHECK(fabsf(song_sec_to_beat(s, song_beat_to_sec(s, 17.5f)) - 17.5f) < 1e-4f);
}

static void test_routine_structure(void) {
    const Routine *r = routine_jazz_solo();
    // steps are in order, contiguous and never overlap
    for (int i = 1; i < r->num_steps; i++) {
        const Step *a = &r->steps[i - 1], *b = &r->steps[i];
        CHECK(b->beat > a->beat);
        CHECK(fabsf(a->beat + a->beats - b->beat) < 1e-4f);
    }
    const Step *last = &r->steps[r->num_steps - 1];
    CHECK(last->move == MV_FINISH);
    CHECK(fabsf(last->beat + last->beats - r->song->total_beats) < 1e-4f);
    // all the required move types are present
    int seen[MV_COUNT] = {0};
    for (int i = 0; i < r->num_steps; i++) seen[r->steps[i].move]++;
    for (int m = 0; m < MV_COUNT; m++) CHECK(seen[m] > 0);
}

static void test_poses(void) {
    const Routine *r = routine_jazz_solo();
    // entrance starts off-centre and ends near centre
    Pose p0 = routine_pose(r, 0.0f), p6 = routine_pose(r, 6.0f);
    CHECK(p0.x < -100.0f);
    CHECK(fabsf(p6.x) < 1.0f);
    // the dancer never leaves the stage once on it, and every frame index is valid
    for (float b = 6.0f; b <= 62.0f; b += 0.05f) {
        Pose p = routine_pose(r, b);
        CHECK(p.x >= -110.0f && p.x <= 110.0f);
        CHECK(p.frame >= 0 && p.frame < FR_COUNT);
        CHECK(p.lift >= 0.0f && p.lift <= 23.0f);
    }
    // leap at beat 20 is airborne mid-move; kick at 12 shows the kick frame
    CHECK(routine_pose(r, 20.85f).frame == FR_LEAP);
    CHECK(routine_pose(r, 20.85f).lift > 15.0f);
    CHECK(routine_pose(r, 12.4f).frame == FR_KICK);
    CHECK(routine_pose(r, 61.9f).frame == FR_FINISH);
    // same beat -> same pose (Watch and Play share the timeline)
    Pose a = routine_pose(r, 33.3f), b = routine_pose(r, 33.3f);
    CHECK(a.frame == b.frame && a.x == b.x);
    int eight, count;
    routine_count(r, 3.2f, &eight, &count); CHECK(eight == 0 && count == 4);
    routine_count(r, 8.0f, &eight, &count); CHECK(eight == 1 && count == 1);
    routine_count(r, 55.9f, &eight, &count); CHECK(eight == 6 && count == 8);
}

static void test_cue_density(void) {
    const Routine *r = routine_jazz_solo();
    CueTrack e, n, h;
    cuetrack_build(&e, r, DIFF_EASY);
    cuetrack_build(&n, r, DIFF_NORMAL);
    cuetrack_build(&h, r, DIFF_HARD);
    CHECK(e.n > 5 && e.n < n.n && n.n < h.n);
    printf("  cues: easy=%d normal=%d hard=%d\n", e.n, n.n, h.n);
    for (int i = 1; i < h.n; i++) CHECK(h.cues[i].time - h.cues[i - 1].time > 2 * WINDOW_GOOD); // windows never overlap
}

static void test_judging(void) {
    const Routine *r = routine_jazz_solo();
    CueTrack t;
    cuetrack_build(&t, r, DIFF_HARD);
    int idx;
    Cue c0 = t.cues[0];
    CHECK(cuetrack_press(&t, c0.button, c0.time + 0.02f, &idx) == J_PERFECT && idx == 0);
    Cue c1 = t.cues[1];
    CHECK(cuetrack_press(&t, c1.button, c1.time - 0.07f, &idx) == J_GREAT);
    Cue c2 = t.cues[2];
    CHECK(cuetrack_press(&t, c2.button, c2.time + 0.12f, &idx) == J_GOOD);
    CHECK(t.combo == 3);
    // stray press far from any cue is not judged
    CHECK(cuetrack_press(&t, CUE_A, t.cues[3].time - 0.3f, &idx) == J_NONE && idx == -1);
    // wrong button on a cue = miss, breaks combo
    Cue c3 = t.cues[3];
    CueButton wrong = (CueButton)((c3.button + 1) % CUE_COUNT);
    CHECK(cuetrack_press(&t, wrong, c3.time, &idx) == J_MISS);
    CHECK(t.combo == 0);
    CHECK(cuetrack_step_missed(&t, c3.step_index));
    // letting cue 4 pass entirely registers a miss automatically
    CHECK(cuetrack_update(&t, t.cues[4].time + WINDOW_GOOD + 0.01f) == 1);
    CHECK(t.cues[4].result == J_MISS);
    // a cue can't be judged twice
    CHECK(cuetrack_press(&t, c0.button, c0.time, &idx) == J_NONE);
    // finish the song: everything resolved, accuracy within bounds
    cuetrack_update(&t, 31.0f);
    int judged = 0;
    for (int i = 0; i < t.n; i++) judged += t.cues[i].result != J_NONE;
    CHECK(judged == t.n);
    float acc = cuetrack_accuracy(&t);
    CHECK(acc > 0.0f && acc < 0.2f);
}

static void play_perfect(CueTrack *t) { int i; for (int k = 0; k < t->n; k++) cuetrack_press(t, t->cues[k].button, t->cues[k].time, &i); }

static void test_scoring_and_progression(void) {
    Studio s; studio_new(&s);
    float watch[CAT_COUNT], play[CAT_COUNT], sloppy[CAT_COUNT];
    CueTrack good, bad;
    cuetrack_build(&good, routine_jazz_solo(), DIFF_NORMAL); play_perfect(&good);
    cuetrack_build(&bad, routine_jazz_solo(), DIFF_NORMAL); cuetrack_update(&bad, 31.0f);
    studio_player_categories(&s, MODE_WATCH, DIFF_NORMAL, NULL, watch);
    studio_player_categories(&s, MODE_PLAY, DIFF_NORMAL, &good, play);
    studio_player_categories(&s, MODE_PLAY, DIFF_NORMAL, &bad, sloppy);
    CHECK(play[CAT_MUSICALITY] > watch[CAT_MUSICALITY]);   // skill matters in Play Mode
    CHECK(sloppy[CAT_MUSICALITY] < play[CAT_MUSICALITY]);
    CHECK(sloppy[CAT_PERFORMANCE] < play[CAT_PERFORMANCE]);

    // training visibly changes later scoring
    char msg[64];
    float before = watch[CAT_TECHNIQUE];
    studio_apply(&s, ACT_TRAIN, msg, sizeof msg);
    CHECK(s.week == 2 && s.funds == 1000 - TRAIN_COST);
    studio_player_categories(&s, MODE_WATCH, DIFF_NORMAL, NULL, watch);
    CHECK(watch[CAT_TECHNIQUE] > before);
    // tiredness blocks actions
    s.dancer.energy = 10;
    CHECK(studio_cannot(&s, ACT_TRAIN) != NULL);
    CHECK(studio_cannot_compete(&s) != NULL);
    CHECK(studio_cannot(&s, ACT_REST) == NULL);
    studio_apply(&s, ACT_REST, msg, sizeof msg);
    CHECK(s.dancer.energy == 50);
}

static void test_competition(void) {
    Studio s; studio_new(&s);
    float cat[CAT_COUNT];
    studio_player_categories(&s, MODE_WATCH, DIFF_NORMAL, NULL, cat);
    Results r1, r2;
    compete_run(&r1, &s, cat, 1234);
    compete_run(&r2, &s, cat, 1234);
    CHECK(memcmp(&r1, &r2, sizeof r1) == 0); // deterministic for a seed
    CHECK(r1.n == 6);
    int places_seen[7] = {0};
    for (int i = 0; i < r1.n; i++) {
        const Entry *e = &r1.e[r1.order[i]];
        CHECK(e->place >= 1 && e->place <= 6);
        places_seen[e->place]++;
        if (i > 0) CHECK(r1.e[r1.order[i - 1]].total_tenths >= e->total_tenths);
    }
    CHECK(places_seen[1] >= 1);
    printf("  watch-mode debut (week 1):\n");
    for (int i = 0; i < r1.n; i++) {
        const Entry *e = &r1.e[r1.order[i]];
        printf("    %d. %-16s %5.1f%s\n", e->place, e->name, e->total_tenths / 10.0, e->is_player ? "  <- you" : "");
    }

    // a perfect Play Mode run on Hard should beat the Watch Mode debut
    CueTrack t; cuetrack_build(&t, routine_jazz_solo(), DIFF_HARD); play_perfect(&t);
    float pc[CAT_COUNT]; studio_player_categories(&s, MODE_PLAY, DIFF_HARD, &t, pc);
    Results rp; compete_run(&rp, &s, pc, 1234);
    CHECK(rp.e[0].place < r1.e[0].place);
    printf("  perfect play-mode run places %d (score %.1f)\n", rp.e[0].place, rp.e[0].total_tenths / 10.0);

    // payouts + progression
    int funds = s.funds, week = s.week;
    Reward w = studio_apply_results(&s, &rp);
    CHECK(s.week == week + 1);
    CHECK(s.funds == funds - ENTRY_FEE + w.money);
    CHECK(s.comps_entered == 1);
    CHECK(s.rival_growth[4] > s.rival_growth[0]); // rivals improve
}

static void test_ties(void) {
    // Force two entries to be identical by giving the player a rival's exact
    // categories, then search seeds until the judges' noise lands equal.
    Studio s; studio_new(&s);
    int found_tie = 0;
    for (uint32_t seed = 1; seed < 20000 && !found_tie; seed++) {
        Results r;
        float cat[CAT_COUNT];
        studio_player_categories(&s, MODE_WATCH, DIFF_NORMAL, NULL, cat);
        compete_run(&r, &s, cat, seed);
        for (int i = 1; i < r.n; i++) {
            const Entry *a = &r.e[r.order[i - 1]], *b = &r.e[r.order[i]];
            if (a->total_tenths == b->total_tenths) {
                // equal totals: either a technique tiebreak decided it, or it's a shared place
                if (a->place == b->place) CHECK(a->tied && b->tied);
                else CHECK(lroundf(a->cat[CAT_TECHNIQUE] * 10) >= lroundf(b->cat[CAT_TECHNIQUE] * 10));
                found_tie = 1;
            }
        }
    }
    CHECK(found_tie);
}

static void test_save_roundtrip(void) {
    Studio s, t; studio_new(&s);
    s.week = 9; s.funds = 1337; s.dancer.stats.technique = 77; s.timing_offset_ms = -20;
    SaveBlob b; save_pack(&s, &b);
    CHECK(save_unpack(&b, &t) == 0);
    CHECK(memcmp(&s, &t, sizeof s) == 0);
    b.studio.funds = 999999; // tamper
    CHECK(save_unpack(&b, &t) == -3);
    save_pack(&s, &b); b.magic = 0; CHECK(save_unpack(&b, &t) == -1);
    save_pack(&s, &b); b.version = 99; CHECK(save_unpack(&b, &t) == -2);
}

static void test_wav(void) {
    uint8_t hdr[48] = {0};
    memcpy(hdr, "RIFF", 4); memcpy(hdr + 8, "WAVE", 4);
    memcpy(hdr + 12, "fmt ", 4); hdr[16] = 16;
    hdr[20] = 1; hdr[22] = 1;                      // PCM, mono
    uint32_t rate = 22050; memcpy(hdr + 24, &rate, 4);
    hdr[34] = 16;                                  // bits
    memcpy(hdr + 36, "data", 4); hdr[40] = 4;
    WavInfo w;
    CHECK(wav_parse(hdr, sizeof hdr, &w) == WAV_OK);
    CHECK(w.sample_rate == 22050 && w.channels == 1 && w.bits == 16 && w.data_offset == 44 && w.data_bytes == 4);
    hdr[34] = 8; CHECK(wav_parse(hdr, sizeof hdr, &w) == WAV_ERR_FORMAT);
    hdr[34] = 16; hdr[20] = 3; CHECK(wav_parse(hdr, sizeof hdr, &w) == WAV_ERR_NOT_PCM);
    CHECK(wav_parse((const uint8_t *)"ID3xxxxxxxxxxx", 14, &w) == WAV_ERR_NOT_RIFF);
}

static void test_real_song_file(void) {
    FILE *f = fopen("../romfs/music/spotlight_sass_30.wav", "rb");
    CHECK(f != NULL);
    if (!f) return;
    static uint8_t buf[2 * 1024 * 1024];
    size_t len = fread(buf, 1, sizeof buf, f);
    fclose(f);
    WavInfo w;
    CHECK(wav_parse(buf, len, &w) == WAV_OK);
    float secs = w.data_bytes / 2.0f / w.sample_rate;
    CHECK(w.sample_rate == 22050 && w.channels == 1 && w.bits == 16);
    CHECK(fabsf(secs - 30.0f) < 0.6f);
    printf("  spotlight_sass_30.wav: %u Hz, %.2f s, %u bytes\n", w.sample_rate, secs, w.data_bytes);
}

static void test_roster(void) {
    int counts[GRP_COUNT] = {0};
    for (int id = 1; id <= ROSTER_SIZE; id++) {
        const RosterSlot *s = roster_slot(id);
        CHECK(s && s->id == id);
        counts[s->group]++;
        if (id > 1) CHECK(s->art == ART_MISSING); // no invented art
    }
    CHECK(counts[GRP_GIRLS] == 8 && counts[GRP_MINIS] == 5 && counts[GRP_MALE] == 4 && counts[GRP_FEMALE] == 8);
    CHECK(roster_slot(17)->group == GRP_MALE && roster_slot(18)->group == GRP_FEMALE);
    CHECK(roster_slot(0) == NULL && roster_slot(26) == NULL);
}

int main(void) {
    test_song_timing();
    test_routine_structure();
    test_poses();
    test_cue_density();
    test_judging();
    test_scoring_and_progression();
    test_competition();
    test_ties();
    test_save_roundtrip();
    test_wav();
    test_real_song_file();
    test_roster();
    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
