#include "game/roster.h"
#include <stddef.h>

// 01-08 girls, 09-13 minis, 14-17 male dancers, 18-25 additional female dancers.
// Only dancer 01 has art, and it is a PLACEHOLDER until the real sheet lands.
static const RosterSlot SLOTS[ROSTER_SIZE] = {
    {1, GRP_GIRLS, "Avery Lane", ART_PLACEHOLDER, "romfs:/gfx/dancer01.t3x"},
    {2, GRP_GIRLS, NULL, ART_MISSING, NULL},  {3, GRP_GIRLS, NULL, ART_MISSING, NULL},
    {4, GRP_GIRLS, NULL, ART_MISSING, NULL},  {5, GRP_GIRLS, NULL, ART_MISSING, NULL},
    {6, GRP_GIRLS, NULL, ART_MISSING, NULL},  {7, GRP_GIRLS, NULL, ART_MISSING, NULL},
    {8, GRP_GIRLS, NULL, ART_MISSING, NULL},
    {9, GRP_MINIS, NULL, ART_MISSING, NULL},  {10, GRP_MINIS, NULL, ART_MISSING, NULL},
    {11, GRP_MINIS, NULL, ART_MISSING, NULL}, {12, GRP_MINIS, NULL, ART_MISSING, NULL},
    {13, GRP_MINIS, NULL, ART_MISSING, NULL},
    {14, GRP_MALE, NULL, ART_MISSING, NULL},  {15, GRP_MALE, NULL, ART_MISSING, NULL},
    {16, GRP_MALE, NULL, ART_MISSING, NULL},  {17, GRP_MALE, NULL, ART_MISSING, NULL},
    {18, GRP_FEMALE, NULL, ART_MISSING, NULL}, {19, GRP_FEMALE, NULL, ART_MISSING, NULL},
    {20, GRP_FEMALE, NULL, ART_MISSING, NULL}, {21, GRP_FEMALE, NULL, ART_MISSING, NULL},
    {22, GRP_FEMALE, NULL, ART_MISSING, NULL}, {23, GRP_FEMALE, NULL, ART_MISSING, NULL},
    {24, GRP_FEMALE, NULL, ART_MISSING, NULL}, {25, GRP_FEMALE, NULL, ART_MISSING, NULL},
};

const RosterSlot *roster_slot(int id) {
    return (id >= 1 && id <= ROSTER_SIZE) ? &SLOTS[id - 1] : NULL;
}

const char *roster_group_name(RosterGroup g) {
    static const char *n[GRP_COUNT] = {"Girls 01-08", "Minis 09-13", "Male Dancers 14-17", "Female Dancers 18-25"};
    return (g >= 0 && g < GRP_COUNT) ? n[g] : "?";
}

const char *art_status_name(ArtStatus a) {
    switch (a) {
    case ART_FINAL: return "Final art";
    case ART_PLACEHOLDER: return "Placeholder art";
    default: return "Art missing";
    }
}

const char *teacher_role(int idx) {
    static const char *r[TEACHER_SLOTS] = {"Ballet & Technique", "Jazz / Jazz Funk", "Hip-Hop & Majorette",
                                           "Contemporary / Lyrical"};
    return (idx >= 0 && idx < TEACHER_SLOTS) ? r[idx] : "?";
}
