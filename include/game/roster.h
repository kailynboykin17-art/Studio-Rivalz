// Studio Rivalz — fixed roster slots. IDs never get renumbered.
#pragma once

typedef enum { GRP_GIRLS = 0, GRP_MINIS, GRP_MALE, GRP_FEMALE, GRP_COUNT } RosterGroup;
typedef enum { ART_MISSING = 0, ART_PLACEHOLDER, ART_FINAL } ArtStatus;

typedef struct {
    int id;             // 1..25
    RosterGroup group;
    const char *name;   // NULL until the character is designed
    ArtStatus art;
    const char *sheet;  // romfs sprite sheet path, NULL if none
} RosterSlot;

#define ROSTER_SIZE 25
#define TEACHER_SLOTS 4

const RosterSlot *roster_slot(int id); // 1-based
const char *roster_group_name(RosterGroup g);
const char *art_status_name(ArtStatus a);
const char *teacher_role(int idx);     // 0..3
