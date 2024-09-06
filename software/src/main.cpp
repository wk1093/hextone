#include <iostream>
#include <cstring>
#include <vector>
#include <thread>
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#endif

#ifdef _WIN32
#define BAUD_RATE CBR_115200
#else
#define BAUD_RATE B115200
#endif

#define ARDUINO_WAIT_TIME 2000

// linux: https://tldp.org/HOWTO/Serial-Programming-HOWTO/x115.html
// windows: https://www.delftstack.com/howto/cpp/cpp-serial-communication/
struct SerialPort { // parity and stuff is not supported, always set to arduino default
#ifdef _WIN32
    HANDLE hSerial;
    bool connected;
    COMSTAT status;
    DWORD errors;
#else
#error "POSIX Support WIP"
#endif

    SerialPort(const char *portName) {
#ifdef _WIN32
        hSerial = CreateFileA(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hSerial == INVALID_HANDLE_VALUE) {
            connected = false;
            return;
        }
        
        DCB dcbSerialParams = {0};

        //Try to get the current
        if (!GetCommState(this->hSerial, &dcbSerialParams)) {
            //If impossible, show an error
            printf("failed to get current serial parameters!");
            CloseHandle(hSerial);
            connected = false;
            return;
        } else {
            dcbSerialParams.BaudRate=CBR_115200;
            dcbSerialParams.ByteSize=8;
            dcbSerialParams.StopBits=ONESTOPBIT;
            dcbSerialParams.Parity=NOPARITY;
            dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;

             if(!SetCommState(hSerial, &dcbSerialParams)) {
                printf("ALERT: Could not set Serial Port parameters");
             } else {
                 this->connected = true;
                 PurgeComm(this->hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
                 Sleep(ARDUINO_WAIT_TIME);
             }
        }
#else
#error "POSIX Support WIP"
#endif
    }

    ~SerialPort() {
        if (connected) {
            CloseHandle(hSerial);
            connected = false;
            return;
        }
    }

    int ReadData(char* buffer, unsigned int nbChar) {
        DWORD bytesRead;
        unsigned int toRead;

        ClearCommError(this->hSerial, &this->errors, &this->status);

        if(this->status.cbInQue>0) {

            if(this->status.cbInQue>nbChar) {
                toRead = nbChar;
            } else {
                toRead = this->status.cbInQue;
            }

            if(ReadFile(this->hSerial, buffer, toRead, &bytesRead, NULL) ) {
                return bytesRead;
            }

        }
        return 0;
    }

    bool WriteData(const char* buffer, unsigned int nbChar) {
        DWORD bytesSend;

        if(!WriteFile(this->hSerial, (void *)buffer, nbChar, &bytesSend, 0)) {
            ClearCommError(this->hSerial, &this->errors, &this->status);
            return false;
        } else {
            return true;
        }
    }
};

int main() {
    SerialPort serialPort("COM6"); // TODO: search for port automatically
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
    std::cout << "ready" << std::endl;
    
    while (true) {
        readSize = serialPort.ReadData(buffer, 1024);
        if (readSize > 0) {
            std::string str(buffer, readSize);
            std::cout << str << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    }

    return 0;




}