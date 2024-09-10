#pragma once

#include <portaudio.h>
#include <vector>
#include <algorithm>
#include <iostream>

#define SAMPLE_RATE 44100

typedef std::vector<float> AudioBuffer;

void initAudio();

void terminateAudio();

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
