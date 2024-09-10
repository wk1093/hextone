#pragma once

#include <portaudio.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cstdint>

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
