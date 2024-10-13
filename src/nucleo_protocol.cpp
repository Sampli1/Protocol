#include "nucleo_protocol.hpp"

// Constructor

Protocol::Protocol(uint8_t version, uint8_t sub_version, uint8_t address, bool verbose) {
    version = version;
    sub_version = sub_version;
    address = address;
    serial.connect_serial();
    serial.set_verbose(verbose);
}

// Utils

uint8_t calculate_CRC_8(const std::vector<uint8_t>& data) {
    uint8_t crc = 0x00;
    uint8_t polynomial = 0x07; 
    for (auto byte : data) {
        crc ^= byte; 
        for (int i = 0; i < 8; i++) {
            if (crc & 0x80) crc = (crc << 1) ^ polynomial;
            else crc <<= 1;
        }
    }
    return crc;
}

uint8_t verify_response_CRC_8(const std::vector<uint8_t>& res, uint8_t start_byte, uint crc_index) {
    // Find pattern in array sequence

    auto val = std::find(res.begin(), res.end(), start_byte);
    
    if (val == res.end()) return COMM_STATUS::NUCLEO_INVALID_ANSWER;

    unsigned int index = std::distance(res.begin(), val);
 
    uint8_t res_crc = res[index + crc_index];
    std::vector<uint8_t> res_to_verify(res.begin() + index, res.begin() + index + crc_index);
    
    // for (int i = 0; i < res_to_verify.size() ; i++) std::cout <<std::to_string(res_to_verify[i]) << " ";
    
    uint8_t crc_to_verify = calculate_CRC_8(res_to_verify);


    return res_crc != crc_to_verify ? COMM_STATUS::CRC_FAILED : COMM_STATUS::OK; 
}

// Public functions

bool Protocol::connect() {
    if (serial.check_connection()) return true;
    return serial.connect_serial();
}

uint8_t Protocol::init(uint8_t frequency) {
    if (!serial.check_connection()) return COMM_STATUS::SERIAL_NOT_ESTABLISHED;

    std::vector<uint8_t> packet = {INIT_SEQ, address, version, sub_version, frequency};
    uint8_t crc = calculate_CRC_8(packet);
    packet.push_back(crc);
    packet.push_back(END_SEQ_INIT);

    // ! TO REMOVE -> ARDUINO NEEDS DELAY TO ESTABLISH SERIAL CONNECTION, NUCLEO HOW MUCH (?)
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    serial.send_byte_array(packet);

    std::vector<uint8_t> res = get_packet(TIMEOUT, INIT_SEQ, END_SEQ_INIT);
 
    uint8_t status = verify_response_CRC_8(res, INIT_SEQ, 5);
    
    return status;
}

void Protocol::send_packet(uint8_t command, uint16_t *packet_array, size_t packet_array_length) {
    if (!serial.check_connection()) return;
    
    std::vector<uint8_t> packet = {COMM_SEQ, command, address};

    for (int i = 0; i < packet_array_length; i++) {
        uint8_t high = packet_array[i] >> 8;
        uint8_t low = (packet_array[i] & 0x00FF);
        packet.push_back(high);
        packet.push_back(low);
    }

    uint8_t crc = calculate_CRC_8(packet);
    
    packet.push_back(crc);
    packet.push_back(END_SEQ_COMM);

    serial.send_byte_array(packet);
}



std::vector<uint8_t> Protocol::get_packet(uint timeout, uint8_t start_byte, uint8_t end_byte) {
    std::vector<uint8_t> res;
    if (!serial.check_connection()) {
        res.push_back(COMM_STATUS::SERIAL_NOT_ESTABLISHED);
        return res;
    }
    
    auto start = std::chrono::high_resolution_clock::now(); 
    bool z = false;
    while (!z && serial.get_available_data() == 0) {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        z = (duration.count() >= timeout);
    }

    if (z) {
        res.push_back(COMM_STATUS::NUCLEO_TIMEOUT);
        return res;
    } 

    res = serial.get_byte_vector(end_byte);

    if (verify_response_CRC_8(res, start_byte, 3) != COMM_STATUS::OK) {
        res.clear();
        res.push_back(COMM_STATUS::CRC_FAILED);
    }

    return res;
}