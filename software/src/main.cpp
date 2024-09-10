#include <iostream>
#include <cstring>
#include <vector>
#include <thread>
#include <chrono>
#include "serial.h"
#include "synth.h"

// TODO: split audio.h into source and header

#define TIMEOUT 2000

int main() {
    initAudio();
    AudioPlayer audioPlayer;
    Synth* synth = new SineSynth();
    // TODO: search for port automatically
    SerialPort serialPort("COM6");
    if (!serialPort.connected) {
        std::cout << "Could not connect to device, exiting" << std::endl;
        return 0;
    }
    char buffer[1024];
    int readSize;
    while (true) {
        readSize = serialPort.ReadData(buffer, 1024);
        if (readSize >= 7) {
            if (strncmp(buffer, "waiting", 7) == 0) {
                break;
            }
            if (strncmp(buffer, "msg", 3) == 0) {
                std::cout << "ERROR: Device is already online, please reboot device and try again" << std::endl;
                return 0;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    serialPort.WriteData("ready", 5);
    // after we write ready, it should respond with "ready"
    while (true) {
        readSize = serialPort.ReadData(buffer, 1024);
        if (readSize >= 5) {
            if (strncmp(buffer, "ready", 5) == 0) {
                break;
            }
        }
    }
    std::cout << "Connected to device" << std::endl;
    audioPlayer.data.buffer = synth->generateSeconds(1.0f);
    audioPlayer.play();

    
    // keep track of time of last message, if it's been too long, kill the connection
    std::chrono::time_point<std::chrono::system_clock> lastMessageTime = std::chrono::system_clock::now();
    

    while (true) {
        readSize = serialPort.ReadData(buffer, 1024);
        if (readSize > 0) {
            std::string str(buffer, readSize);
            if (str.find("msg") != std::string::npos) {
                std::cout << str << std::endl;
                lastMessageTime = std::chrono::system_clock::now();
                continue;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastMessageTime).count() > TIMEOUT) {
            // the device was probably powered off, close the connection
            std::cout << "Device disconnected, closing connection" << std::endl;
            return 0;
        }
    }

    terminateAudio();
    return 0;
}