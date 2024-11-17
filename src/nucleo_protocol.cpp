#include "nucleo_protocol.hpp"

// Constructor

Protocol::Protocol(uint8_t version, uint8_t sub_version, uint8_t address, int baudrate, bool verbose) {
    m_version = version;
    m_sub_version = sub_version;
    m_address = address;
    m_raw_buffer = {};
    m_buffer = {};
    

    m_serial.set_baudrate(baudrate);
    m_serial.set_verbose(verbose);
    m_serial.connect_serial();
}

// Utils -> Private

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

uint8_t verify_response_CRC_8(const std::vector<uint8_t>& res, size_t actual_crc_index) {
    uint8_t res_crc = res[actual_crc_index];

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
    for (int i = vec.size() - 1; i > 0; i--) {
        if (std::find(bytes_to_escape, bytes_to_escape + NUM_SEQ + 2, vec[i]) != bytes_to_escape + NUM_SEQ + 2) {
            vec.insert(vec.begin() + i, ESCAPE_CHAR);    
        }
    }
}

keys_t get_keys(std::unordered_map<uint8_t, packet_t> buffer) {
    keys_t available_keys;
    for (auto pair : buffer) available_keys.push_back(pair.first);
    return available_keys;
}

// Function that returns the actual index of a packet may be affected by byte stuffing
ssize_t actual_index(std::vector<uint8_t>& raw_packet, size_t index) {
    ssize_t actual_index = -1;
    for (int i = 0; i < raw_packet.size(); i++) {
        if (raw_packet[i] == ESCAPE_CHAR) {
            i++;
            if (i >= raw_packet.size()) break;
        }
        actual_index++;
        if (index == actual_index) return i;
    }
    return -1;
}

// ! URGE TO REVIEW 
bool complete_packet(std::vector<uint8_t> packet, std::unordered_map<uint8_t, packet_t>& buffer) {
    size_t dim = packet.size();
    if (dim < 3) return false;

    bool is_valid = std::find(start_bytes, start_bytes + NUM_SEQ, packet[0]) != start_bytes + NUM_SEQ // Is a start of packet?
    && (packet[dim - 1] == END_SEQ // Ends with END_SEQ?
    && (packet[dim - 2] != ESCAPE_CHAR // Is there ESCAPE_CHAR?
    || (packet[dim - 2] == ESCAPE_CHAR && packet[dim - 3] == ESCAPE_CHAR))); // There is no ESCAPING SEQUENCE?

    if (is_valid) {
        size_t actual_crc_index = actual_index(packet, 3);
        if (actual_crc_index == -1) return false;
        if (verify_response_CRC_8(packet, actual_crc_index) != COMM_STATUS::OK) buffer[packet[0]] = {COMM_STATUS::CRC_FAILED, packet};
        else buffer[packet[0]] = {COMM_STATUS::OK, packet};
    }
    return is_valid;
} 

// Public functions

bool Protocol::is_connected() {
    return m_serial.check_connection();
}

bool Protocol::connect() {
    if (m_serial.check_connection()) return true;
    return m_serial.connect_serial();
}

COMM_STATUS Protocol::init(uint8_t interval, uint8_t max_retries, uint8_t time_between_retries) {

    if (!m_serial.check_connection()) return COMM_STATUS::SERIAL_NOT_ESTABLISHED;

    std::vector<uint8_t> packet = {INIT_SEQ, m_address, m_version, m_sub_version, interval};

    uint8_t crc = calculate_CRC_8(packet);
    packet.push_back(crc);
    packet.push_back(END_SEQ);
 

    // ! TO REMOVE -> ARDUINO NEEDS DELAY TO ESTABLISH SERIAL CONNECTION, NUCLEO HOW MUCH (?)
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    
    ssize_t written_bytes = m_serial.send_byte_array(packet);
    
    if (written_bytes == -1) return COMM_STATUS::SERIAL_NOT_ESTABLISHED;    

    size_t retry = 0;
    keys_t available_keys = update_buffer();
    while (std::find(available_keys.begin(), available_keys.end(), INIT_SEQ) == available_keys.end() && retry < max_retries) {
        std::this_thread::sleep_for(std::chrono::milliseconds(time_between_retries));
        available_keys = update_buffer();
        retry++;
    }


    packet_t res = get_packet(INIT_SEQ, END_SEQ);
 
    if (res.first != COMM_STATUS::OK) return res.first; 

    std::vector<uint8_t> res_val = res.second.value();
    size_t status_index = actual_index(res_val, 4);

    if (status_index == -1) return COMM_STATUS::NUCLEO_INVALID_ANSWER;

    return static_cast<COMM_STATUS>(res_val[status_index]);
}

ssize_t Protocol::send_packet(uint8_t command, uint16_t *packet_array, size_t packet_array_length) {
    if (!m_serial.check_connection()) return -1;
    
    std::vector<uint8_t> packet = {COMM_SEQ, command, m_address};

    // Transform uint16_t -> uint8_t
    for (int i = 0; i < packet_array_length; i++) {
        uint8_t high = packet_array[i] >> 8;
        uint8_t low = (packet_array[i] & 0x00FF);
        packet.push_back(high);
        packet.push_back(low);
    }

    uint8_t crc = calculate_CRC_8(packet);
    

    // TODO change order
    packet.push_back(crc);

    // Byte stuffing 
    add_escape_char(packet);
    
    packet.push_back(END_SEQ);

    return m_serial.send_byte_array(packet);
}

packet_t Protocol::get_packet(uint8_t start_byte, uint8_t end_byte) {
    if (!m_serial.check_connection()) return {COMM_STATUS::SERIAL_NOT_ESTABLISHED, std::nullopt}; 
    
    // In order to avoid this type of error, use update_buffer keys in start_byte
    if (m_buffer.find(start_byte) == m_buffer.end()) return {COMM_STATUS::SERIAL_NOT_IN_BUFFER, std::nullopt};

    packet_t packet = m_buffer[start_byte];

    m_buffer.erase(start_byte);

    return packet;
}

void print_vec__(std::vector<uint8_t> val) {
    for (int i = 0; i < val.size(); i++) std::cout << std::hex << static_cast<int>(val[i]) << " ";
    std::cout << std::dec << std::endl;
}

keys_t Protocol::update_buffer() { 
    std::vector<std::vector<uint8_t>> new_buffer = m_serial.get_byte_vectors(END_SEQ, ESCAPE_CHAR);
    for (std::vector<uint8_t> packet: new_buffer) {
        // Complete packet
        if (!complete_packet(packet, m_buffer)) {
            // Incomplete packet => Bufferize waiting the other packet to complete or complete previous incomplete packet 
            m_raw_buffer.insert(m_raw_buffer.end(), packet.begin(), packet.end());
            // Try to complete
            std::cout << "BUFFER_RAW: dim: " <<  m_raw_buffer.size() << std::endl;
            print_vec__(m_raw_buffer);

            if (complete_packet(m_raw_buffer, m_buffer) ) {
               m_raw_buffer.clear();
                std::cout << "REBUILDED" << std::endl;
            }
            if (m_raw_buffer.size() > 20) m_raw_buffer.clear();
        }
    }

    return get_keys(m_buffer);
}

void Protocol::disconnect() {
    m_serial.disconnect_serial();
}

Protocol::~Protocol() {
    disconnect();
}