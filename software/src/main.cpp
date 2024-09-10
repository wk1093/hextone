#include <iostream>
#include <cstring>
#include <vector>
#include <thread>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "serial.h"
#include "synth.h"

// TODO: split audio.h into source and header

#define TIMEOUT 2000

// there is going to be a thread that is responsible for recieveing the data from the device, and putting that data into a queue
// the main loo (that handles audio and graphics) will read from that queue and play the data whenever it can (skipping over data, if it builds up too much)

struct QueueItem {
    enum class Type {
        NoteOn,
        NoteOff,
        Shutdown
    };
    Type type;
    // uint8_t x;
    // uint8_t y;
    // uint8_t note;
    // uint8_t velocity;
    // just a byte array for now
    std::vector<uint8_t> data;
};

struct Queue {
    std::queue<QueueItem> queue;
    std::mutex mutex;
    std::condition_variable conditionVariable;
    void push(QueueItem item) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(item);
        conditionVariable.notify_one();
    }
    QueueItem pop() {
        std::unique_lock<std::mutex> lock(mutex);
        conditionVariable.wait(lock, [this] { return !queue.empty(); });
        QueueItem item = queue.front();
        queue.pop();
        return item;
    }
    [[nodiscard]] bool empty() {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.empty();
    }
};

struct GlobalState {
    Queue queue;
    std::thread queueThread;
    SerialPort* serialPort;
    bool exit = false;
};

GlobalState globalState;

void queueThreadFunction() {
    char buffer[1024]; // commands should only be a few bytes long, but this is just in case
    std::chrono::time_point<std::chrono::system_clock> lastMessageTime = std::chrono::system_clock::now();
    while (true) {
        int readSize = globalState.serialPort->ReadData(buffer, 1024);
        if (readSize > 0) {
            std::string str(buffer, readSize);
            if (str.find("msg") != std::string::npos) {
                QueueItem item;
                // o: note on
                // f: note off
                // s: shutdown
                // other: error
                if (str[str.find("msg")+4] == 'o') {
                    item.type = QueueItem::Type::NoteOn;
                    item.data = std::vector<uint8_t>(str.begin() + str.find("msg") + 5, str.end()-1);
                } else if (str[str.find("msg")+4] == 'f') {
                    item.type = QueueItem::Type::NoteOff;
                    item.data = std::vector<uint8_t>(str.begin() + str.find("msg") + 5, str.end()-1);
                } else if (str[str.find("msg")+4] == 's') {
                    item.type = QueueItem::Type::Shutdown;
                    item.data = std::vector<uint8_t>(str.begin() + str.find("msg") + 5, str.end()-1);
                } else {
                    std::cerr << "Error: Unknown message type " << str[str.find("msg")+4] << std::endl;
                }
                globalState.queue.push(item);
            } else {
                std::cout << "WARNING: Mangled message" << std::endl;
            }
            lastMessageTime = std::chrono::system_clock::now();
        }
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastMessageTime).count() > TIMEOUT) {
            // the device was probably powered off, close the connection
            std::cout << "Device disconnected, closing connection" << std::endl;
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    globalState.exit = true;
}

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

    
    globalState.serialPort = &serialPort;
    globalState.queueThread = std::thread(queueThreadFunction);

    while (true) {
        if (globalState.exit) {
            break;
        }
        if (globalState.queue.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        QueueItem item = globalState.queue.pop();
        std::cout << "Received item of type " << (int)item.type << std::endl;
        std::cout << "Data: " << std::endl;
        for (uint8_t byte : item.data) {
            std::cout << (char)byte;
        }
        std::cout << std::endl;
        
    }

    terminateAudio();
    return 0;
}