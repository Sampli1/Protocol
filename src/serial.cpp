#include <iostream>
#include <chrono>
#include <thread>
#include "serial.hpp"
#include "wiringSerial.h"

using namespace std;
const string serial_prefix =  "/dev/ttyACM";

Serial::Serial() {
    this->connected = false;
    this->device = "";
    this->fd = -1;
}

int Serial::connect_serial()
{
    string serial_inteface;
    int max_tries = 4;

    cout << "[SERIAL] Trying connecting to serial..." << endl;

    // Try to connect to differet serial interfaces
    for (int i = 0; i < max_tries && this->fd == -1; i++) {
        serial_inteface = serial_prefix + std::to_string(i);
        this->fd = serialOpen(serial_inteface.c_str(), 9600);
        if (this->fd != 2) {
            cout << "[SERIAL] " << "Connected to:" << serial_inteface << endl;
            this->connected = true;
            this->device = serial_inteface;
        }
    }

    cout << "[SERIAL] " << this->connected << endl;
    serialFlush(this->fd);
    return this->connected;
}

int Serial::get_available_data() {
    return serialDataAvail(this->fd);   
}


string Serial::get_line(char terminal) {
    string msg = "";
    while (serialDataAvail(this->fd) > 0) {
        char z = serialGetchar(this->fd);
        msg.push_back(z);
        if (z == terminal) break;
        this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return msg;
}

void Serial::send_line(string msg) {
    serialPuts(this->fd, msg.c_str());
}


