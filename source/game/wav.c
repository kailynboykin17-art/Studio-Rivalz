#include "game/wav.h"
#include <string.h>

static uint32_t rd32(const uint8_t *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

int wav_parse(const uint8_t *buf, size_t len, WavInfo *out) {
    memset(out, 0, sizeof(*out));
    if (len < 12) return WAV_ERR_SHORT;
    if (memcmp(buf, "RIFF", 4) != 0 || memcmp(buf + 8, "WAVE", 4) != 0) return WAV_ERR_NOT_RIFF;
    size_t pos = 12;
    int have_fmt = 0;
    while (pos + 8 <= len) {
        const uint8_t *ck = buf + pos;
        uint32_t sz = rd32(ck + 4);
        if (memcmp(ck, "fmt ", 4) == 0) {
            if (sz < 16 || pos + 8 + 16 > len) return WAV_ERR_SHORT;
            uint16_t fmt = rd16(ck + 8);
            if (fmt != 1) return WAV_ERR_NOT_PCM;
            out->channels = rd16(ck + 10);
            out->sample_rate = rd32(ck + 12);
            out->bits = rd16(ck + 22);
            if (out->bits != 16 || out->channels < 1 || out->channels > 2 ||
                out->sample_rate < 8000 || out->sample_rate > 48000)
                return WAV_ERR_FORMAT;
            have_fmt = 1;
        } else if (memcmp(ck, "data", 4) == 0) {
            if (!have_fmt) return WAV_ERR_FORMAT;
            out->data_offset = (uint32_t)(pos + 8);
            out->data_bytes = sz;
            if (out->data_offset + out->data_bytes > len) out->data_bytes = (uint32_t)(len - out->data_offset);
            return out->data_bytes > 0 ? WAV_OK : WAV_ERR_NO_DATA;
        }
        pos += 8 + sz + (sz & 1);
    }
    return have_fmt ? WAV_ERR_NO_DATA : WAV_ERR_NOT_PCM;
}

const char *wav_error(int code) {
    switch (code) {
    case WAV_OK: return "OK";
    case WAV_ERR_SHORT: return "File is too short or cut off";
    case WAV_ERR_NOT_RIFF: return "Not a WAV file";
    case WAV_ERR_NOT_PCM: return "Unsupported format. Convert to 16-bit PCM WAV";
    case WAV_ERR_FORMAT: return "Unsupported format. Use 16-bit mono PCM WAV, 22050 Hz";
    case WAV_ERR_NO_DATA: return "WAV has no audio data";
    default: return "Unknown WAV error";
    }
}
