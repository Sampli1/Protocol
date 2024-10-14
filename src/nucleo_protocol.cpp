#include "nucleo_protocol.hpp"

// Constructor

Protocol::Protocol(uint8_t version, uint8_t sub_version, uint8_t address, int baudrate, bool verbose) {
    m_version = version;
    m_sub_version = sub_version;
    m_address = address;

    serial.set_baudrate(baudrate);
    serial.set_verbose(verbose);
    serial.connect_serial();
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

uint8_t verify_response_CRC_8(const std::vector<uint8_t>& res, uint8_t start_byte) {
    // Find pattern in array sequence

    auto val = std::find(res.begin(), res.end(), start_byte);
    
    if (val == res.end()) return COMM_STATUS::NUCLEO_INVALID_ANSWER;

    unsigned int index = std::distance(res.begin(), val);
 
    uint8_t res_crc = res[res.size() - 2];
    std::vector<uint8_t> res_to_verify(res.begin() + index, res.begin() + res.size() - 2);
    
    // for (int i = 0; i < res_to_verify.size() ; i++) std::cout <<std::to_string(res_to_verify[i]) << " ";
    
    uint8_t crc_to_verify = calculate_CRC_8(res_to_verify);


    return res_crc != crc_to_verify ? COMM_STATUS::CRC_FAILED : COMM_STATUS::OK; 
}

// Public functions

bool Protocol::is_connected() {
    return serial.check_connection();
}

bool Protocol::connect() {
    if (serial.check_connection()) return true;
    return serial.connect_serial();
}

uint8_t Protocol::init(uint8_t frequency) {
    if (!serial.check_connection()) return COMM_STATUS::SERIAL_NOT_ESTABLISHED;

    std::vector<uint8_t> packet = {INIT_SEQ, m_address, m_version, m_sub_version, frequency};

    uint8_t crc = calculate_CRC_8(packet);
    packet.push_back(crc);
    packet.push_back(END_SEQ_INIT);

    // ! TO REMOVE -> ARDUINO NEEDS DELAY TO ESTABLISH SERIAL CONNECTION, NUCLEO HOW MUCH (?)
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    ssize_t written_bytes = serial.send_byte_array(packet);

    if (written_bytes == -1) return COMM_STATUS::SERIAL_NOT_ESTABLISHED;

    std::pair<uint8_t ,std::optional<std::vector<uint8_t>>> res = get_packet(TIMEOUT, INIT_SEQ, END_SEQ_INIT);
 
    if (res.first != COMM_STATUS::OK) return res.first; 

    return res.second.value()[res.second.value().size() - 3];
}

ssize_t Protocol::send_packet(uint8_t command, uint16_t *packet_array, size_t packet_array_length) {
    if (!serial.check_connection()) return -1;
    
    std::vector<uint8_t> packet = {COMM_SEQ, command, m_address};

    for (int i = 0; i < packet_array_length; i++) {
        uint8_t high = packet_array[i] >> 8;
        uint8_t low = (packet_array[i] & 0x00FF);
        packet.push_back(high);
        packet.push_back(low);
    }

    uint8_t crc = calculate_CRC_8(packet);
    
    packet.push_back(crc);
    packet.push_back(END_SEQ_COMM);

    return serial.send_byte_array(packet);
}

std::pair<uint8_t ,std::optional<std::vector<uint8_t>>> Protocol::get_packet(uint timeout, uint8_t start_byte, uint8_t end_byte) {
    
    if (!serial.check_connection()) return {COMM_STATUS::SERIAL_NOT_ESTABLISHED, std::nullopt};
    
    
    auto start = std::chrono::high_resolution_clock::now(); 
    bool timeout_occurred = false;
    
    while (!timeout_occurred && serial.get_available_data() == 0) {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        timeout_occurred = (duration.count() >= timeout);
    }



    if (timeout_occurred) return {COMM_STATUS::NUCLEO_TIMEOUT, std::nullopt};

    std::vector<uint8_t> res = serial.get_byte_vector(end_byte);

    if (verify_response_CRC_8(res, start_byte) != COMM_STATUS::OK) return {COMM_STATUS::CRC_FAILED, std::nullopt}; 
    
    return {COMM_STATUS::OK, res};
}