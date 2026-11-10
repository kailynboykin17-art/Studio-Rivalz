// Minimal RIFF/WAVE header validation (PCM 16-bit, mono or stereo).
#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t sample_rate;
    uint16_t channels;
    uint16_t bits;
    uint32_t data_offset; // byte offset of first sample
    uint32_t data_bytes;
} WavInfo;

enum { WAV_OK = 0, WAV_ERR_SHORT = -1, WAV_ERR_NOT_RIFF = -2, WAV_ERR_NOT_PCM = -3,
       WAV_ERR_FORMAT = -4, WAV_ERR_NO_DATA = -5 };

int wav_parse(const uint8_t *buf, size_t len, WavInfo *out);
const char *wav_error(int code);
