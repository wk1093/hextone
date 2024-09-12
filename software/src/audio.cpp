#include "audio.h"

void initAudio() {
    Pa_Initialize();
}

void terminateAudio() {
    Pa_Terminate();
}

int AudioPlayer::callback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) {
    auto* data = (AudioPlayerData*)userData;
    if (data->position >= data->buffer.size()) {
        for (unsigned long i = 0; i < framesPerBuffer; i++) {
            ((float*)outputBuffer)[i] = 0.0f;
        }
        return paContinue; // keep playing silence
    }
    float* out = (float*)outputBuffer;
    for (unsigned long i = 0; i < framesPerBuffer; i++) {
        if (data->position < data->buffer.size()) {
            *out++ = data->buffer[data->position++];
        } else {
            *out++ = 0.0f;
        }
    }
    return paContinue;
}

AudioPlayer::AudioPlayer() {
    // PaError e = Pa_OpenDefaultStream(&stream, 0, 1, paFloat32, SAMPLE_RATE, paFramesPerBufferUnspecified, callback, &data);
    // manual stream
    int defaultDevice = Pa_GetDefaultOutputDevice();
    PaStreamParameters outputParameters;
    outputParameters.device = defaultDevice;
    outputParameters.channelCount = 1;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = 0.05; // NOTE: lower number is less latency, but lower quality and more CPU usage
    outputParameters.hostApiSpecificStreamInfo = nullptr;

    PaError e = Pa_OpenStream(&stream, nullptr, &outputParameters, SAMPLE_RATE, paFramesPerBufferUnspecified, paNoFlag, callback, &data);
    if (e != paNoError) {
        std::cerr << "Error: PortAudio failed to open stream" << std::endl;
        return;
    }
}

AudioPlayer::AudioPlayer(const AudioBuffer& buffer) : AudioPlayer() {
    data.buffer = buffer;
}

AudioPlayer::~AudioPlayer() {
    PaError e = Pa_CloseStream(stream);
    if (e != paNoError) {
        std::cerr << "Error: PortAudio failed to close stream" << std::endl;
        std::cerr << Pa_GetErrorText(e) << std::endl;
    }
}

void AudioPlayer::play() {
    // prevent errors
    // if (data.buffer.empty()) {
    //     std::cerr << "Error: AudioPlayer::play called with empty buffer" << std::endl;
    //     return;
    // }
    // make sure there is no corruption in the buffer
    if (data.position >= data.buffer.size()) {
        data.position = 0;
    }
    // if already playing, do nothing
    PaError active = Pa_IsStreamActive(stream);
    if (active == 1) {
        return;
    }
    PaError e = Pa_StartStream(stream);
    if (e != paNoError) {
        std::cerr << "Error: PortAudio failed to start stream" << std::endl;
        std::cerr << Pa_GetErrorText(e) << std::endl;
        return;
    }
}

void AudioPlayer::pause() const {
    PaError stopped = Pa_IsStreamStopped(stream);
    if (stopped == 1) {
        return;
    }
    PaError e = Pa_StopStream(stream);
    if (e != paNoError) {
        std::cerr << "Error: PortAudio failed to stop stream" << std::endl;
        std::cerr << Pa_GetErrorText(e) << std::endl;
        return;
    }
}

void AudioPlayer::resume() const {
    PaError stopped = Pa_IsStreamStopped(stream);
    if (stopped == 0) {
        return;
    }
    PaError e = Pa_StartStream(stream);
    if (e != paNoError) {
        std::cerr << "Error: PortAudio failed to start stream" << std::endl;
        std::cerr << Pa_GetErrorText(e) << std::endl;
        return;
    }
}

void AudioPlayer::stop() {
    PaError stopped = Pa_IsStreamStopped(stream);
    if (stopped == 1) {
        data.position = 0;
        return;
    }
    PaError e = Pa_AbortStream(stream);
    data.position = 0;
    if (e != paNoError) {
        std::cerr << "Error: PortAudio failed to stop stream" << std::endl;
        std::cerr << Pa_GetErrorText(e) << std::endl;
        return;
    }
}

void AudioPlayer::seek(float progress) {
    if (progress < 0.0f) {
        progress = 0.0f;
    } else if (progress > 1.0f) {
        progress = 1.0f;
    }
    size_t newPosition = static_cast<size_t>(progress * data.buffer.size());
    data.position = newPosition;
    if (data.position >= data.buffer.size()) {
        data.position = data.buffer.size() - 1;
        return;
    }
    if (data.position < 0) {
        data.position = 0;
        return;
    }
}

void AudioPlayer::mixNow(AudioBuffer buffer) {
    // mix the buffer into the current buffer at the current position so it starts playing, if it is at the end of the buffer when this is called, it will delete the entire buffer and replce it with this
    if (data.position >= data.buffer.size()) {
        data.buffer = buffer;
        data.position = 0;
        return;
    }
    if (data.position + buffer.size() > data.buffer.size()) {
        data.buffer.resize(data.position + buffer.size());
    }
    for (size_t i = 0; i < buffer.size(); i++) {
        data.buffer[data.position + i] += buffer[i];
        if (data.buffer[data.position + i] > 1.0f) {
            data.buffer[data.position + i] = 1.0f;
            continue;
        }
        if (data.buffer[data.position + i] < -1.0f) {
            data.buffer[data.position + i] = -1.0f;
            continue;
        }
    }
}

