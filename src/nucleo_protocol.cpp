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

uint8_t verify_response_CRC_8(const std::vector<uint8_t>& res) { 
    uint8_t res_crc = res[res.size() - 2];

    std::vector<uint8_t> res_to_verify;
    
    // These lines exclude ESCAPE_CHAR from CRC calculation
    for (size_t i = 0; i < res.size() - 2; i++) {
        if (res[i] != ESCAPE_CHAR) {
            res_to_verify.push_back(res[i]);
        } 
        else if (i + 1 < res.size() - 2 && res[i + 1] == ESCAPE_CHAR) {
            res_to_verify.push_back(res[i + 1]);
            i++;
        }
    } 


    uint8_t crc_to_verify = calculate_CRC_8(res_to_verify);

    return res_crc != crc_to_verify ? COMM_STATUS::CRC_FAILED : COMM_STATUS::OK; 
}

void add_escape_char(std::vector<uint8_t>& vec) {
    for (int i = 0; i < vec.size(); i++) if (vec[i] == END_SEQ || vec[i] == ESCAPE_CHAR) vec.insert(vec.begin() + (i++), ESCAPE_CHAR);    
}

// Public functions

bool Protocol::is_connected() {
    return serial.check_connection();
}

bool Protocol::connect() {
    if (serial.check_connection()) return true;
    return serial.connect_serial();
}

COMM_STATUS Protocol::init(uint8_t frequency) {
    if (!serial.check_connection()) return COMM_STATUS::SERIAL_NOT_ESTABLISHED;

    std::vector<uint8_t> packet = {INIT_SEQ, m_address, m_version, m_sub_version, frequency};

    uint8_t crc = calculate_CRC_8(packet);
    packet.push_back(crc);
    packet.push_back(END_SEQ);

    // ! TO REMOVE -> ARDUINO NEEDS DELAY TO ESTABLISH SERIAL CONNECTION, NUCLEO HOW MUCH (?)
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    ssize_t written_bytes = serial.send_byte_array(packet);

    if (written_bytes == -1) return COMM_STATUS::SERIAL_NOT_ESTABLISHED;

    std::pair<COMM_STATUS ,std::optional<std::vector<uint8_t>>> res = get_packet(TIMEOUT, INIT_SEQ, END_SEQ);
 
    if (res.first != COMM_STATUS::OK) return res.first; 

    return static_cast<COMM_STATUS>(res.second.value()[res.second.value().size() - 3]);
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
    add_escape_char(packet);
    
    packet.push_back(END_SEQ);


    return serial.send_byte_array(packet);
}

std::pair<COMM_STATUS ,std::optional<std::vector<uint8_t>>> Protocol::get_packet(uint timeout, uint8_t start_byte, uint8_t end_byte) {
    
    if (!serial.check_connection()) return {COMM_STATUS::SERIAL_NOT_ESTABLISHED, std::nullopt};
    
    
    auto start = std::chrono::high_resolution_clock::now(); 
    bool timeout_occurred = false;
    
    while (!timeout_occurred && serial.get_available_data() == 0) {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        timeout_occurred = (duration.count() >= timeout);
    }



    if (timeout_occurred) return {COMM_STATUS::NUCLEO_TIMEOUT, std::nullopt};

    std::vector<uint8_t> res = serial.get_byte_vector(start_byte, end_byte, ESCAPE_CHAR);

    if (res.size() == 0) return {COMM_STATUS::NUCLEO_INVALID_ANSWER, std::nullopt};

    if (verify_response_CRC_8(res) != COMM_STATUS::OK) return {COMM_STATUS::CRC_FAILED, std::nullopt}; 

    return {COMM_STATUS::OK, res};
}