// This main.cpp is for demonstration purposes only
#include <iostream>
#include "nucleo_protocol.hpp"

#define ADDRESS 0x00
#define VERSION 0x01
#define SUB_VERSION 0x01
#define INTERVAL_KEY 0x00
#define BAUDRATE 115200


void print_vec_(std::vector<uint8_t> val) {
    for (int i = 0; i < val.size(); i++) std::cout << std::hex << static_cast<int>(val[i]) << " ";
    std::cout << std::dec << std::endl;
}


// Function which handles disconnection
void handle_disconnection(Protocol &p) {
    if (!p.is_connected()) {
        p.connect();
        while (p.init(INTERVAL_KEY) != COMM_STATUS::OK) {
            std::cout << "INIT FAILED" << std::endl;
            p.connect();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        };
        std::cout << "INIT SUCCESS" << std::endl;
    }
}


int main(int argc, char **argv) {
    // Init variables

    protocol_config_t chimpanzee_config {
        .address = ADDRESS,
        .version = VERSION,
        .sub_version = SUB_VERSION,
        .baudrate = BAUDRATE,
        .verbose = true,
    };

    Protocol p(VERSION, SUB_VERSION, ADDRESS, BAUDRATE, true); // -> Is going to be deprecated
    // Protocol p(chimpanzee_config);

    uint16_t p_motor[8] = { 0xEEEE, 0x2230, 0xffff, 0xaabb, 0xdead, 0xbeef, 0xaabb, 0x7E7E };
    uint16_t p_arm[1] = { 0xEEEE };
    
    // Sensors environment

    sensor_config_t temperature {
        .id = 0x00,
        .i2c_address = 0x22,
        .interval_key = 0x00,
        .type = SENSOR_TYPE::TEMPERATURE
    };

    sensor_config_t flood {
        .id = 0x01,
        .i2c_address = 0x30,
        .interval_key = 0x00,
        .type = SENSOR_TYPE::FLOOD
    };

    // INIT

    while (p.init(INTERVAL_KEY) != COMM_STATUS::OK) {
        p.connect();
        std::cout << "INIT FAILED" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "INIT SUCCESS" << std::endl;


    // SET POLLING SESSION
    p.set_sensor(temperature);
    p.set_sensor(flood);

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
        packet_t sensor_1 = p.get_sensor(temperature.id);
        packet_t sensor_2 = p.get_sensor(flood.id);
        
        
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
            if (i % 200 == 0) std::cout << "PROFILE [" << i << "/5] TO END" << std::endl;
            if (i == 1000) break;
        }
        i++;
    }

    return 0;    
}