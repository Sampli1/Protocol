#include "nucleo_protocol.hpp"

void print_vec__(std::vector<uint8_t> val) {
    for (int i = 0; i < val.size(); i++) std::cout << std::hex << static_cast<int>(val[i]) << " ";
    std::cout << std::dec << std::endl;
}
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
    for (int i = vec.size() - 1; i > 0; i--) {
        if (std::find(bytes_to_escape, bytes_to_escape + NUM_SEQ + 2, vec[i]) != bytes_to_escape + NUM_SEQ + 2) {
            vec.insert(vec.begin() + i, ESCAPE_CHAR);    
        }
    }
}

void remove_escape_char(std::vector<uint8_t>& input) {
    size_t write_index = 0;
    for (size_t read_index = 0; read_index < input.size(); read_index++) {
        if (input[read_index] == ESCAPE_CHAR && read_index + 1 < input.size()) {
            input[write_index++] = input[++read_index];
        } else {
            input[write_index++] = input[read_index];
        }
    }
    input.resize(write_index);
}

keys_t get_keys(std::unordered_map<uint8_t, packet_t> buffer) {
    keys_t available_keys;
    for (auto pair : buffer) if (pair.first != RESERVED_BUFFER_ENTRY) available_keys.push_back(pair.first);
    return available_keys;
}

bool is_valid_packet(std::vector<uint8_t> packet) {
    size_t dim = packet.size();
    return std::find(start_bytes, start_bytes + NUM_SEQ, packet[0]) != start_bytes + NUM_SEQ // Is a start of packet?
    && (packet[dim - 1] == END_SEQ // Ends with END_SEQ?
    && (packet[dim - 2] != ESCAPE_CHAR // Is there ESCAPE_CHAR?
    || (packet[dim - 2] == ESCAPE_CHAR && packet[dim - 3] == ESCAPE_CHAR))); // There is no ESCAPING SEQUENCE?
}

void handle_packet_presentation(std::unordered_map<uint8_t, packet_t> &buffer, std::vector<uint8_t> &packet, std::vector<uint8_t> &entries) {
    uint8_t entry, start_index, end_index;
    // Purify packet
    remove_escape_char(packet);
    // Decide presentation according to packet
    switch (packet[0]) {
        case INIT_SEQ:
            entry = packet[0];
            start_index = 0;
            end_index = packet.size() - 2;
            break;
        case COMM_SEQ:  
            entry = packet[3]; 
            start_index = 3;
            end_index = packet.size() - 3;
            break;
        case HB_SEQ:
            entry = packet[0];
            start_index = 2;
            end_index = packet.size() - 3;
            break;
    }
    
    // Is in the map already?
    if (std::find(entries.begin(), entries.end(), entry) != entries.end()) return; 
    
    // Is CRC 8 correct?
    if (verify_response_CRC_8(packet) != COMM_STATUS::OK) buffer[entry] = { COMM_STATUS::CRC_FAILED, std::nullopt };

    // THE PACKET IS READY
    std::vector<uint8_t> slice(packet.begin() + start_index, packet.begin() + end_index);

    buffer[entry] = { COMM_STATUS::OK, slice }; 
}

void handle_buffer(std::unordered_map<uint8_t, packet_t> &buffer, std::vector<uint8_t> &packet, std::vector<uint8_t> &entries) {
    if (buffer.find(RESERVED_BUFFER_ENTRY) != buffer.end()) {
        std::vector<uint8_t> buffer_entry = buffer[RESERVED_BUFFER_ENTRY].second.value();
        if (buffer_entry.size() > 0) {
            buffer_entry.insert(buffer_entry.end(), packet.begin(), packet.end());
            if (is_valid_packet(buffer_entry)) {
                std::cout << "\nrebuilded" << std::endl;
                print_vec__(buffer_entry);
                handle_packet_presentation(buffer, buffer_entry, entries);
            }
        }
    }
    else buffer[RESERVED_BUFFER_ENTRY] = {COMM_STATUS::OK, packet};
}

void filter_latest(std::unordered_map<uint8_t, packet_t> &buffer, std::vector<std::vector<uint8_t>> &packets) {
    std::vector<uint8_t> entries = {};
    for (int i = packets.size() - 1; i >= 0; i--) {
        if (is_valid_packet(packets[i])) handle_packet_presentation(buffer, packets[i], entries);
        else handle_buffer(buffer, packets[i], entries);
    }   
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
    
    // Build INIT packet
    std::vector<uint8_t> packet = {INIT_SEQ, m_address, m_version, m_sub_version, interval};
    packet.push_back(calculate_CRC_8(packet));
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
        std::cout << "KEYS" << std::endl;
        print_vec__(available_keys);
        retry++;
    }

    packet_t res = get_packet(INIT_SEQ, END_SEQ);
 
    if (res.first != COMM_STATUS::OK) return res.first; 

    std::vector<uint8_t> res_val = res.second.value();

    // TODO: to implements version/subversion check


    return static_cast<COMM_STATUS>(res_val[4]); // 4 -> init index
}

ssize_t Protocol::send_packet(uint8_t command, uint16_t *packet_array, size_t packet_array_length) {
    if (!m_serial.check_connection()) return -1;
    
    std::vector<uint8_t> packet = {COMM_SEQ, command, m_address};

    // Transform uint16_t -> 2 uint8_t
    for (int i = 0; i < packet_array_length; i++) {
        uint8_t high = packet_array[i] >> 8;
        uint8_t low = (packet_array[i] & 0x00FF);
        packet.push_back(high);
        packet.push_back(low);
    }

    uint8_t crc = calculate_CRC_8(packet);
    
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



keys_t Protocol::update_buffer(bool raw) { 
    std::vector<std::vector<uint8_t>> packets = m_serial.get_byte_vectors(END_SEQ, ESCAPE_CHAR);

    std::cout << "ciao" << std::endl;
    filter_latest(m_buffer, packets);

    return get_keys(m_buffer);
}

void Protocol::disconnect() {
    m_serial.disconnect_serial();
}

Protocol::~Protocol() {
    disconnect();
}