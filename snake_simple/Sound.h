#pragma once
// ============================================================
//  Sound.h
//  Generates all sound effects in code using SDL2_mixer.
//  No audio files needed — pure math.
//
//  How it works:
//    1. Fill a buffer of Sint16 samples (PCM audio)
//    2. Apply a simple volume envelope (attack + decay)
//    3. Wrap it in an SDL Mix_Chunk and store by name
// ============================================================
#include <SDL2/SDL_mixer.h>
#include <cmath>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>

class Sound {
public:
    static const int RATE = 22050;   // samples per second

    Sound() {
        if (Mix_OpenAudio(RATE, AUDIO_S16SYS, 2, 512) < 0) {
            std::cerr << "Audio unavailable: " << Mix_GetError() << "\n";
            _ok = false;
            return;
        }
        _ok = true;
        _build();
    }

    ~Sound() {
        for (auto& [k,c] : _chunks) Mix_FreeChunk(c);
        Mix_CloseAudio();
    }

    void play(const std::string& name) {
        if (!_ok) return;
        auto it = _chunks.find(name);
        if (it != _chunks.end()) Mix_PlayChannel(-1, it->second, 0);
    }

private:
    bool _ok = false;
    std::unordered_map<std::string, Mix_Chunk*> _chunks;

    // ── waveform generators ──────────────────────────────────
    // Returns a stereo Sint16 buffer for the given wave.
    // wave: "sine" | "square" | "saw"
    // freq: Hz, dur: seconds, vol: 0..1
    // attack/decay: fraction of total duration for envelope ramp
    static std::vector<Sint16> makeTone(float freq, float dur,
                                        const char* wave = "sine",
                                        float vol = 0.4f,
                                        float attack = 0.01f,
                                        float decay  = 0.15f) {
        int n = (int)(RATE * dur);
        std::vector<Sint16> buf(n * 2);  // stereo = 2 channels
        int atk = (int)(attack * n);
        int dec = (int)(decay  * n);

        for (int i = 0; i < n; i++) {
            float t     = (float)i / RATE;
            float phase = 2.f * M_PI * freq * t;

            // Waveform
            float raw = 0;
            if      (!strcmp(wave,"sine"))   raw = sinf(phase);
            else if (!strcmp(wave,"square")) raw = sinf(phase) >= 0 ? 1.f : -1.f;
            else if (!strcmp(wave,"saw"))    raw = 2.f*(t*freq - floorf(t*freq+.5f));

            // Envelope: ramp up, ramp down, silence
            float env = 0;
            if      (i < atk)       env = (float)i / std::max(atk,1);
            else if (i < atk + dec) env = 1.f - (float)(i-atk) / std::max(dec,1);

            Sint16 s = (Sint16)(raw * env * vol * 32767.f);
            buf[i*2] = buf[i*2+1] = s;   // same sample for L and R
        }
        return buf;
    }

    // Frequency sweep from f0 → f1 (good for whoosh / death sounds)
    static std::vector<Sint16> makeSweep(float f0, float f1, float dur,
                                          const char* wave = "sine",
                                          float vol = 0.4f) {
        int n = (int)(RATE * dur);
        std::vector<Sint16> buf(n * 2);
        float phase = 0;
        for (int i = 0; i < n; i++) {
            float t    = (float)i / n;
            float freq = f0 + (f1 - f0) * t;   // linear sweep
            phase += 2.f * M_PI * freq / RATE;
            float raw = (!strcmp(wave,"square"))
                        ? (sinf(phase) >= 0 ? 1.f : -1.f) : sinf(phase);
            float env = 1.f - t;                // fade out
            Sint16 s = (Sint16)(raw * env * vol * 32767.f);
            buf[i*2] = buf[i*2+1] = s;
        }
        return buf;
    }

    // ── wrap buffer into an SDL Mix_Chunk ────────────────────
    Mix_Chunk* toChunk(const std::vector<Sint16>& buf) {
        int bytes    = (int)buf.size() * sizeof(Sint16);
        auto* data   = new Uint8[bytes];
        std::memcpy(data, buf.data(), bytes);
        Mix_Chunk* c = new Mix_Chunk();
        c->abuf      = data;
        c->alen      = (Uint32)bytes;
        c->allocated = 1;
        c->volume    = MIX_MAX_VOLUME;
        return c;
    }

    // ── build all sound effects ──────────────────────────────
    void _build() {
        //            name          freq   dur    wave     vol   atk    dec
        _chunks["eat"]      = toChunk(makeTone(660, 0.12f, "sine",   0.5f, 0.005f, 0.10f));
        _chunks["poison"]   = toChunk(makeSweep(400, 120, 0.3f, "square", 0.4f));
        _chunks["die"]      = toChunk(makeSweep(300,  50, 0.6f, "saw",    0.5f));
        _chunks["win"]      = toChunk(makeSweep(440, 900, 0.8f, "sine",   0.45f));
        _chunks["obstacle"] = toChunk(makeSweep(500, 180, 0.25f,"square", 0.4f));
        _chunks["nav"]      = toChunk(makeTone(440, 0.07f, "sine",  0.25f, 0.003f, 0.06f));
        _chunks["select"]   = toChunk(makeSweep(440, 880, 0.2f, "sine",   0.35f));
    }
};
