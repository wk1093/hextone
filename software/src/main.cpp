#include <iostream>
#include <cstring>
#include <vector>
#include <thread>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "serial.h"
#include "synth.h"


#define TIMEOUT 2000

// there is going to be a thread that is responsible for recieveing the data from the device, and putting that data into a queue
// the main loo (that handles audio and graphics) will read from that queue and play the data whenever it can (skipping over data, if it builds up too much)

struct QueueItem {
    enum class Type {
        NoteOn,
        NoteOff,
        Shutdown,
        Ping
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
        if (globalState.exit) {
            break;
        }
        int readSize = globalState.serialPort->ReadData(buffer, 1024);
        if (readSize > 0) {
            std::string str(buffer, readSize);
            if (str.find("msg") != std::string::npos) {
                QueueItem item;
                // o: note on
                // f: note off
                // s: shutdown
                // p: ping
                // other: error
                while (str.find("msg(") != std::string::npos) {
                    if (str[str.find("msg")+4] == 'o') {
                        item.type = QueueItem::Type::NoteOn;
                    } else if (str[str.find("msg")+4] == 'f') {
                        item.type = QueueItem::Type::NoteOff;
                    } else if (str[str.find("msg")+4] == 'p') {
                        item.type = QueueItem::Type::Ping;
                    } else if (str[str.find("msg")+4] == 's') {
                        item.type = QueueItem::Type::Shutdown;
                    } else {
                        std::cerr << "Error: Unknown message type " << str[str.find("msg")+4] << " in " << str << std::endl;
                    }
                    item.data = std::vector<uint8_t>(str.begin() + str.find("msg") + 5, str.begin()+str.find(')'));
                    str = str.substr(str.find(')')+1);
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
            globalState.exit = true;
            return;
        }
    }
    globalState.exit = true;
}

int main() {
    initAudio();

    EnvelopeSynth* synth = new EnvelopeSynth();
    synth->amplitude = 0.25f;
    synth->waveType = EnvelopeSynth::WaveType::Square;
    // TODO: search for port automatically
    SerialPort serialPort("COM6");
    if (!serialPort.connected) {
        std::cout << "Could not connect to device, exiting" << std::endl;
        return 0;
    }

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Hexagonal", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Error: Failed to create window" << std::endl;
        return 0;
    }
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    char buffer[1024];
    int readSize;
    while (true) {
        glfwPollEvents();
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
        glfwPollEvents();
        readSize = serialPort.ReadData(buffer, 1024);
        if (readSize >= 5) {
            if (strncmp(buffer, "ready", 5) == 0) {
                break;
            }
        }
    }
    std::cout << "Connected to device" << std::endl;
    AudioStream stream;
    // play a basic startup sound (3 notes over the span of 3/4 seconds)
    // true minor chord (using microtonal scale)
    synth->frequency = 440;
    stream.write(synth->generateSeconds(0.25f), AudioOffset::fromSeconds(0.0f));
    synth->frequency = 440*1.333;
    stream.write(synth->generateSeconds(0.25f), AudioOffset::fromSeconds(0.25f));
    synth->frequency = 440*1.333*1.333;
    stream.write(synth->generateSeconds(0.25f), AudioOffset::fromSeconds(0.5f));

    AudioPlayer* audioPlayer = new AudioPlayer(stream.buffer);
    audioPlayer->play();

    
    globalState.serialPort = &serialPort;
    globalState.queueThread = std::thread(queueThreadFunction);

    bool running = true;

    std::vector<bool> buttonStates(8, false);


    // float notelength = 1.0f;


    // synth->frequency = 440;
    // AudioBuffer presynth0 = synth->generateSeconds(notelength);
    // AudioPlayer* player0 = new AudioPlayer(presynth0);

    // synth->frequency = synth->frequency*1.05946456;
    // AudioBuffer presynth1 = synth->generateSeconds(notelength);
    // AudioPlayer* player1 = new AudioPlayer(presynth1);

    // synth->frequency = synth->frequency*1.05946456;
    // AudioBuffer presynth2 = synth->generateSeconds(notelength);
    // AudioPlayer* player2 = new AudioPlayer(presynth2);

    // synth->frequency = synth->frequency*1.05946456;
    // AudioBuffer presynth3 = synth->generateSeconds(notelength);
    // AudioPlayer* player3 = new AudioPlayer(presynth3);

    // synth->frequency = synth->frequency*1.05946456;
    // AudioBuffer presynth4 = synth->generateSeconds(notelength);
    // AudioPlayer* player4 = new AudioPlayer(presynth4);

    // synth->frequency = synth->frequency*1.05946456;
    // AudioBuffer presynth5 = synth->generateSeconds(notelength);
    // AudioPlayer* player5 = new AudioPlayer(presynth5);

    // synth->frequency = synth->frequency*1.05946456;
    // AudioBuffer presynth6 = synth->generateSeconds(notelength);
    // AudioPlayer* player6 = new AudioPlayer(presynth6);

    // synth->frequency = synth->frequency*1.05946456;
    // AudioBuffer presynth7 = synth->generateSeconds(notelength);
    // AudioPlayer* player7 = new AudioPlayer(presynth7);

    // synth->frequency = synth->frequency*1.05946456;
    // AudioBuffer presynth8 = synth->generateSeconds(notelength);
    // AudioPlayer* player8 = new AudioPlayer(presynth8);

    // std::vector<AudioPlayer*> players = {player0, player1, player2, player3, player4, player5, player6, player7, player8};

    AudioSynthesizer* synth1 = new AudioSynthesizer(SineSynthf::synth, nullptr);
    AudioSynthesizer* synth2 = new AudioSynthesizer(SineSynthf::synth, nullptr);
    AudioSynthesizer* synth3 = new AudioSynthesizer(SineSynthf::synth, nullptr);
    AudioSynthesizer* synth4 = new AudioSynthesizer(SineSynthf::synth, nullptr);
    AudioSynthesizer* synth5 = new AudioSynthesizer(SineSynthf::synth, nullptr);
    AudioSynthesizer* synth6 = new AudioSynthesizer(SineSynthf::synth, nullptr);
    AudioSynthesizer* synth7 = new AudioSynthesizer(SineSynthf::synth, nullptr);
    AudioSynthesizer* synth8 = new AudioSynthesizer(SineSynthf::synth, nullptr);
    std::vector<AudioSynthesizer*> synths = {synth1, synth2, synth3, synth4, synth5, synth6, synth7, synth8};
    std::vector<float> frequencies = {440, 460, 480, 500, 520, 540, 560, 580};


    while (running) {
        if (globalState.exit) {
            running = false;
        }

        if (!globalState.queue.empty()) {
            QueueItem item = globalState.queue.pop();
            if (item.type == QueueItem::Type::NoteOn) {
                int button = item.data[0]-'0';
                buttonStates[button] = true;
                synths[button-1]->start(frequencies[button-1], 0.5f);
            } else if (item.type == QueueItem::Type::NoteOff) {
                int button = item.data[0]-'0';
                buttonStates[button] = false;
                synths[button-1]->stop();
            }
        }

        // prerender
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();



        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit")) {
                    globalState.exit = true;
                    running = false;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // button display
        // hexagon that change color when button is pressed

        // imgui render a hexagon

        ImGui::Begin("Hexagonal", nullptr);
        ImVec2 center = ImGui::GetWindowPos()+ImVec2(ImGui::GetWindowSize().x/2, ImGui::GetWindowSize().y/2);

        // ImColor color = ImColor(0, 0, 0, 0);
        // if (buttonStates[0]) {
        //     std::cout << "Button 1 pressed" << std::endl;
        //     color = ImGui::GetStyle().Colors[ImGuiCol_ButtonActive];
        // } else {
        //     color = ImGui::GetStyle().Colors[ImGuiCol_Button];
        // }
        // ImGui::GetWindowDrawList()->AddCircleFilled(center, 10, color, 6);

        // make in a hexagonal pattern, 2 x 4 buttons
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 2; j++) {
                ImColor color = ImColor(0, 0, 0, 0);
                if (buttonStates[(1-j)*4+i+1]) {
                    color = ImGui::GetStyle().Colors[ImGuiCol_ButtonActive];
                } else {
                    color = ImGui::GetStyle().Colors[ImGuiCol_Button];
                }
                ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(center.x+j*30, center.y+i*35-j*15), 20, color, 6);
            }
        }

        ImGui::End();



        // render
        ImGui::Render();


        // postrender
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();

        if (glfwWindowShouldClose(window)) {
            running = false;
            globalState.exit = true;
        }
    }

    globalState.exit = true;

    globalState.queueThread.join();
    
    serialPort.WriteData("shutdown", 8);

    delete synth;
    delete audioPlayer;
    glfwTerminate();
    terminateAudio();
    return 0;
}