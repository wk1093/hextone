#pragma once

#include <cstdio>

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

    SerialPort(const char *portName);

    ~SerialPort();

    int ReadData(char* buffer, unsigned int nbChar);

    bool WriteData(const char* buffer, unsigned int nbChar);
};