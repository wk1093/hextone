#pragma once

#include <portaudio.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cstdint>
#include <cmath>

#define SAMPLE_RATE 44100

typedef std::vector<float> AudioBuffer;

void initAudio();

void terminateAudio();

struct AudioOffset {
    size_t samples = 0;

    AudioOffset() = default;

    static AudioOffset fromSamples(size_t samples) {
        AudioOffset offset;
        offset.samples = samples;
        return offset;
    }

    static AudioOffset fromSeconds(double seconds) {
        AudioOffset offset;
        offset.samples = static_cast<size_t>(seconds * SAMPLE_RATE);
        return offset;
    }

    static AudioOffset fromMilliseconds(int64_t milliseconds) {
        AudioOffset offset;
        offset.samples = static_cast<size_t>(milliseconds * SAMPLE_RATE / 1000);
        return offset;
    }

    AudioOffset operator+(const AudioOffset& other) const {
        AudioOffset offset;
        offset.samples = samples + other.samples;
        return offset;
    }

    AudioOffset operator-(const AudioOffset& other) const {
        AudioOffset offset;
        offset.samples = samples - other.samples;
        return offset;
    }
};

struct AudioStream {
    AudioBuffer buffer{};

    AudioStream() = default;

    void write(const AudioBuffer& buf, AudioOffset offset = AudioOffset::fromSamples(0)) {
        if (offset.samples + buf.size() > buffer.size()) {
            buffer.resize(offset.samples + buf.size());
        }
        for (size_t i = 0; i < buf.size(); i++) {
            // mix the audio
            buffer[i + offset.samples] += buf[i];
            // this get's loud very quickly, so we need to normalize it
            if (buffer[i + offset.samples] > 1.0f) {
                buffer[i + offset.samples] = 1.0f;
            } else if (buffer[i + offset.samples] < -1.0f) {
                buffer[i + offset.samples] = -1.0f;
            }
        }
    }

    void write(const AudioStream& stream, AudioOffset offset = AudioOffset::fromSamples(0)) {
        write(stream.buffer, offset);
    }

    void clear() {
        buffer.clear();
    }
};


struct AudioPlayer {
    struct AudioPlayerData {
        AudioBuffer buffer;
        size_t position = 0;
    } data;

    PaStream* stream{};

    static int callback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);

    AudioPlayer();

    explicit AudioPlayer(const AudioBuffer& buffer);

    ~AudioPlayer();

    void play();

    void pause() const;

    void resume() const;

    void stop();

    void mixNow(AudioBuffer buffer); // mix the buffer into the current buffer at the current position so it starts playing, if it is at the end of the buffer when this is called, it will delete the entire buffer and replce it with this

    [[nodiscard]] inline bool isPlaying() const {
        return Pa_IsStreamActive(stream) == 1;
    }

    inline float progress() const {
        return static_cast<float>(data.position) / data.buffer.size();
    }

    void seek(float progress);

    inline const AudioBuffer& getBuffer() const {
        return data.buffer;
    }

    inline AudioBuffer& getBuffer() {
        return data.buffer;
    }

    inline size_t getPosition() const {
        return data.position;
    }

};

typedef AudioBuffer (*SynthFunction)(size_t sampleCount, float frequency, float amplitude, size_t position, bool isDown, void* userData);

// alternative to AudioPlayer
struct AudioSynthesizer {
    // synthesizes data on the fly whenever it is requested, this allows us to stop a note, and have a fade out, and other effects too
    struct AudioSynthesizerData {
        SynthFunction function;
        void* userData;
        size_t position = 0;
        float frequency = 440;
        float amplitude = 0.5;
        bool isDown = false; // is the key currently pressed

    } data;

    PaStream* stream{};
    static int callback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);
    AudioSynthesizer(SynthFunction function, void* userData);
    ~AudioSynthesizer();
    void start(float frequency, float amplitude);
    void stop();
};

struct SineSynthf {
    static inline AudioBuffer synth(size_t sampleCount, float frequency, float amplitude, size_t position, bool isDown, void* userData) {
        AudioBuffer buffer(sampleCount);
        for (size_t i = 0; i < sampleCount; i++) {
            float t = static_cast<float>(position+i) / SAMPLE_RATE;
            float value = amplitude * std::sin(2.0f * (float)M_PI * frequency * t);
            buffer[i] = value;
            if (isDown) {
                buffer[i] = 0.0f;
                break;
            }
        }
        return buffer;
    }
};


struct SquareSynthf {
    static inline AudioBuffer synth(size_t sampleCount, float frequency, float amplitude, size_t position, bool isDown, void* userData) {
        AudioBuffer buffer(sampleCount);
        for (size_t i = 0; i < sampleCount; i++) {
            float t = static_cast<float>(i) / SAMPLE_RATE;
            float value = amplitude * (std::sin(2.0f * (float)M_PI * frequency * t) > 0.0f ? 1.0f : -1.0f);
            buffer[i] = value;
            if (isDown) {
                buffer[i] = 0.0f;
                break;
            }
        }
        return buffer;
    }
};

template<typename T>
struct AudioSynther {
    AudioSynthesizer* synth;
    T data;
    AudioSynther(T data) : data(data) {
        synth = new AudioSynthesizer(T::synth, &data);
    }
    ~AudioSynther() {
        delete synth;
        synth = nullptr;
    }
    void start(float frequency, float amplitude) {
        synth->start(frequency, amplitude);
    }
    void stop() {
        synth->stop();
    }
};