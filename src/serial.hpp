#ifndef SERIAL_H_
#define SERIAL_H_

#include <iostream>

using namespace std;

class Serial {
    public:
        Serial();
        
        /* Parameters */
        bool get_connected() const { return connected; }
        string get_device() const { return device; }
        int get_fd() const { return fd; }
        
        /* Utils */
        int connect_serial();
        int get_available_data();
        string get_line(char terminal = '\n');
        void send_line(string msg);    
    
    private:
        int fd; // File Descriptor
        bool connected; 
        string device;
};


#endif // SERIAL_H_