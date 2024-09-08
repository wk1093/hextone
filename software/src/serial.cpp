#include "serial.h"

SerialPort::SerialPort(const char *portName) {
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

SerialPort::~SerialPort() {
#ifdef _WIN32
    if (connected) {
        CloseHandle(hSerial);
        connected = false;
        return;
    }
#else
#error "POSIX Support WIP"
#endif
}

int SerialPort::ReadData(char* buffer, unsigned int nbChar) {
#ifdef _WIN32
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
#else
#error "POSIX Support WIP"
#endif
}

bool SerialPort::WriteData(const char* buffer, unsigned int nbChar) {
#ifdef _WIN32
    DWORD bytesSend;

    if(!WriteFile(this->hSerial, (void *)buffer, nbChar, &bytesSend, 0)) {
        ClearCommError(this->hSerial, &this->errors, &this->status);
        return false;
    } else {
        return true;
    }
#else
#error "POSIX Support WIP"
#endif
}