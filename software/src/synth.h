#pragma once

#include "audio.h"
#include <cmath>
#include <cstdint>

typedef std::vector<uint8_t> ByteBuffer;

#define GLOBAL_VOLUME 0.5f // this lets us mix without clipping

// C++17 basic LightDaw Synthesizer

struct Synth { // abstract class
    float sampleRate = 44100;
    float frequency = 440;
    float amplitude = 0.5;
    float volume = 1.0;
    bool open = false;

    inline virtual AudioBuffer generateSamples(size_t sampleCount) = 0;
    inline virtual AudioBuffer generateSeconds(double seconds) {
        return generateSamples(static_cast<size_t>(seconds * sampleRate));
    }

    virtual ~Synth() = default;

    inline static const uint64_t id = 0;
};

struct SineSynth : public Synth {

    inline AudioBuffer generateSamples(size_t sampleCount) override {
        AudioBuffer buffer(sampleCount);
        for (size_t i = 0; i < sampleCount; i++) {
            float t = static_cast<float>(i) / sampleRate;
            float value = amplitude * std::sin(2.0f * (float)M_PI * frequency * t);
            buffer[i] = value;
        }
        return buffer;
    }

    inline static const uint64_t id = 1;
};

struct TriangleSynth : public Synth {

    inline AudioBuffer generateSamples(size_t sampleCount) override {
        AudioBuffer buffer(sampleCount);
        for (size_t i = 0; i < sampleCount; i++) {
            float t = static_cast<float>(i) / sampleRate;
            float value = amplitude * 2.0f * std::abs(2.0f * (frequency * t - std::floor(frequency * t + 0.5f))) - 1.0f;
            buffer[i] = value;
        }
        return buffer;
    }

    inline static const uint64_t id = 2;
};

struct SquareSynth : public Synth {
    inline AudioBuffer generateSamples(size_t sampleCount) override {
        AudioBuffer buffer(sampleCount);
        for (size_t i = 0; i < sampleCount; i++) {
            float t = static_cast<float>(i) / sampleRate;
            float value = amplitude * (std::sin(2.0f * (float)M_PI * frequency * t) > 0.0f ? 1.0f : -1.0f);
            buffer[i] = value;
        }
        return buffer;
    }

    inline static const uint64_t id = 3;
};

struct SawSynth : public Synth {
    inline AudioBuffer generateSamples(size_t sampleCount) override {
        AudioBuffer buffer(sampleCount);
        for (size_t i = 0; i < sampleCount; i++) {
            float t = static_cast<float>(i) / sampleRate;
            float value = amplitude * 2.0f * (frequency * t - std::floor(frequency * t + 0.5f));
            buffer[i] = value;
        }
        return buffer;
    }

    inline static const uint64_t id = 4;
};

struct EnvelopeSynth : public Synth {
    float attack = 0.1f;
    float decay = 0.1f;
    float sustain = 0.5f;
    float release = 0.1f;
    float attackVol = 1.0f;
    float decayVol = 0.5f;
    float releaseVol = 0.0f;
    float currentVol = 0.0f;
    // what to use (sine, triangle, square, saw)
    enum class WaveType {
        Sine,
        Triangle,
        Square,
        Saw
    } waveType = WaveType::Sine;

    inline AudioBuffer generateSamples(size_t sampleCount) override {
        AudioBuffer buffer(sampleCount);
        float time = 0.0f;
        for (size_t i = 0; i < sampleCount; i++) {
            float t = static_cast<float>(i) / sampleRate;
            if (time < attack) {
                currentVol = time / attack * (attackVol - 0.0f) + 0.0f;
            } else if (time < attack + decay) {
                currentVol = (time - attack) / decay * (decayVol - attackVol) + attackVol;
            } else if (time < attack + decay + sustain) {
                currentVol = sustain;
            } else if (time < attack + decay + sustain + release) {
                currentVol = (time - attack - decay - sustain) / release * (releaseVol - sustain) + sustain;
            } else {
                currentVol = 0.0f;
            }
            float value = 0.0f;
            float vol = amplitude * currentVol * GLOBAL_VOLUME;
            switch (waveType) {
                case WaveType::Sine:
                    value = vol * std::sin(2.0f * (float)M_PI * frequency * t);
                    break;
                case WaveType::Triangle:
                    value = vol * 2.0f * std::abs(2.0f * (frequency * t - std::floor(frequency * t + 0.5f))) - 1.0f;
                    break;
                case WaveType::Square:
                    value = vol * (std::sin(2.0f * (float)M_PI * frequency * t) > 0.0f ? 1.0f : -1.0f);
                    break;
                case WaveType::Saw:
                    value = vol * 2.0f * (frequency * t - std::floor(frequency * t + 0.5f));
                    break;
            }
            buffer[i] = value;
            time += 1.0f / sampleRate;
        }
        return buffer;
    }

    inline static const uint64_t id = 5;
};