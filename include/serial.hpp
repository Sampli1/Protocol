#ifndef SERIAL_H_
#define SERIAL_H_

#include <iostream>
#include <vector>
#include <unistd.h>
#include <cstring>

class Serial {
    public:
        Serial();
        
        std::string get_device() const { return m_device; }
        int get_fd() const { return m_fd; }
        void set_verbose(bool val);        



        bool check_connection();
        
        
        /**
        * Connect serial communication, device is assigned automatically
        * @return True: successful connection, False: error in connection
        */
        int connect_serial();
        
        /**
        * Get the amount of incoming bytes from serial communication  
        * 
        */
        int get_available_data();

        
        

        /**
         * Get bytes until terminal byte sequence
         * @param terminal Final byte
         * @return Byte vector
         */
        std::vector<uint8_t> get_byte_vector(unsigned char terminal);

        ssize_t send_byte_array(std::vector<uint8_t> bytes);


    private:
        int m_fd; // File Descriptor
        bool m_connected; 
        std::string m_device;
        bool m_verbose;
};


#endif // SERIAL_H_