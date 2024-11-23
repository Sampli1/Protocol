// This main.cpp is for demonstration purposes only
#include <iostream>
#include "nucleo_protocol.hpp"

#define VERSION 0x01
#define SUB_VERSION 0x01
#define BAUDRATE 115200


void print_vec_(std::vector<uint8_t> val) {
    for (int i = 0; i < val.size(); i++) std::cout << std::hex << static_cast<int>(val[i]) << " ";
    std::cout << std::dec << std::endl;
}

void handle_disconnection(Protocol &p) {
    if (!p.is_connected()) {
        p.connect();
        while (p.init(0x00) != COMM_STATUS::OK) {
            std::cout << "INIT FAILED" << std::endl;
            p.connect();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        };
        std::cout << "INIT SUCCESS" << std::endl;
    }
}


int main(int argc, char **argv) {
    // Init variables
    Protocol p(VERSION, SUB_VERSION, 0x01, BAUDRATE, false);
    uint16_t p_motor[8] = { 0xEEEE, 0x2230, 0xffff, 0xaabb, 0xdead, 0xbeef, 0xaabb, 0x7E7E };
    uint16_t p_arm[1] = { 0xEEEE };
    uint8_t temperature_id = 0;
    uint8_t temperature_i2c_addr = 0x22;
    uint8_t temperature_int = 0;
    uint8_t temperature_type = SENSOR_TYPE::TEMPERATURE;
    
    uint8_t flood_id = 1;
    uint8_t flood_i2c_addr = 0x30;
    uint8_t flood_int = 0;
    uint8_t flood_type = SENSOR_TYPE::FLOOD;


    for (int i = 0; i < argc; i++) {
        std::cout << argv[i] << std::endl;
    }

    // INIT

    while (p.init(0x00) != COMM_STATUS::OK) {
        p.connect();
        std::cout << "INIT FAILED" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "INIT SUCCESS" << std::endl;


    // SET POLLING SESSION
    p.set_sensor(temperature_id, temperature_i2c_addr, temperature_int, temperature_type);
    p.set_sensor(flood_id, flood_i2c_addr, flood_int, flood_type);

    // COMMUNICATION
    keys_t keys;
    COMM_STATUS status;


    int i = 0;
    while (true) {
        handle_disconnection(p);
        p.update_buffer();

        // Send motor data 
        p.send_packet(COMM_TYPE::MOTOR, p_motor, 8);
        p.send_packet(COMM_TYPE::ARM, p_arm, 1);
        

        // Read packet
        packet_t hb = p.get_heartbeat();
        packet_t sensor_1 = p.get_sensor(temperature_id);
        packet_t sensor_2 = p.get_sensor(flood_id);
        
        std::cout << "[HB]" << std::endl;
        if (hb.first == COMM_STATUS::OK) print_vec_(hb.second.value());
        else std::cout << "PROTOCOL ERROR CODE 0x" << hb.first << std::endl;

        std::cout << "[SENSOR_1]" << std::endl;
        if (sensor_1.first == COMM_STATUS::OK) print_vec_(sensor_1.second.value());
        else std::cout << "PROTOCOL ERROR CODE 0x" << sensor_1.first << std::endl;
        
        std::cout << "[SENSOR_2]" << std::endl;
        if (sensor_2.first == COMM_STATUS::OK) print_vec_(sensor_2.second.value());
        else std::cout << "PROTOCOL ERROR CODE 0x" << sensor_2.first << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));


        if (argc > 1 && std::strcmp(argv[1], "profile") == 0) {
            if (i % 200 == 0) std::cout << "PROFILE [" << i++ << "/5] TO END" << std::endl;
            if (i == 1000) break;
        }
    }

    return 0;    
}