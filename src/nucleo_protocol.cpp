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
    m_verbose = verbose;
    m_buffer = {};

    m_serial.set_baudrate(baudrate);
    m_serial.set_verbose(verbose);
    m_serial.connect_serial();
}


void Protocol::handle_packet_presentation(std::vector<uint8_t> &packet, std::vector<uint8_t> &entries) {
    uint8_t entry, start_index, end_index;
    // Purify packet
    remove_escape_char(packet);
    // Decide presentation according to packet
    switch (packet[0]) {
        case INIT_SEQ:
            entry = packet[0];
            start_index = 0;
            break;
        case COMM_SEQ:
            if (packet.size() < 5) return; // NEED A WAY 
            entry = packet[3]; 
            start_index = 3;
            break;
        case HB_SEQ:
            entry = packet[0];
            start_index = 2;
            break;
    }
    end_index = packet.size() - 2;
    
    // Is in the map already?
    if (std::find(entries.begin(), entries.end(), entry) != entries.end()) return; 
    entries.push_back(entry);

    if (m_verbose) {
        std::cout << "[CHIMPANZEE] COLLECT -> " << std::hex << static_cast<int>(entry) << std::dec << std::endl;
    }

    // Is CRC 8 correct?
    if (verify_response_CRC_8(packet) != COMM_STATUS::OK) m_buffer[entry] = { COMM_STATUS::CRC_FAILED, std::nullopt };

    // THE PACKET IS READY
    std::vector<uint8_t> slice(packet.begin() + start_index, packet.begin() + end_index);

    m_buffer[entry] = { COMM_STATUS::OK, slice }; 
}

bool Protocol::handle_buffer(std::vector<uint8_t> &packet, std::vector<uint8_t> &entries) {
    // Is the buffer empty?
    if (m_buffer.find(RESERVED_BUFFER_ENTRY) == m_buffer.end()) {
        m_buffer[RESERVED_BUFFER_ENTRY] = {COMM_STATUS::OK, packet};
        return false;
    }
    // Update the buffer and check validity
    std::vector<uint8_t> buffer_entry = m_buffer[RESERVED_BUFFER_ENTRY].second.value();
    buffer_entry.insert(buffer_entry.end(), packet.begin(), packet.end());
    if (is_valid_packet(buffer_entry)) {
        if (m_verbose) {
            std::cout << "REBUILD" << std::endl;
            print_vec__(buffer_entry);
        }
        m_buffer.erase(RESERVED_BUFFER_ENTRY);
        handle_packet_presentation(buffer_entry, entries);
        return true;
    } 
    m_buffer[RESERVED_BUFFER_ENTRY] = {COMM_STATUS::OK, buffer_entry};
    return false;    
}

void Protocol::filter_latest(std::vector<std::vector<uint8_t>> &packets) {
    if (packets.size() == 0) return;
    
    std::vector<uint8_t> entries = {};
    int end = 0;

    // Rebuild first packet (These lines are important in the unlucky case of 2 or more incomplete packet in stream)
    if (m_buffer.find(RESERVED_BUFFER_ENTRY) != m_buffer.end()) {
        for (int i = 0; i < packets.size(); i++) {
            if (handle_buffer(packets[end++], entries) || m_buffer[RESERVED_BUFFER_ENTRY].second.value().size() > 20) {
                std::cout << "ERASED" << std::endl;
                m_buffer.erase(RESERVED_BUFFER_ENTRY);
                break;        
            }
        }
    }

    entries = {}; 

    // End index to avoid to reconsider the first incomplete packets
    for (int i = packets.size() - 1; i >= end; i--) {
        if (is_valid_packet(packets[i])) handle_packet_presentation(packets[i], entries);
        else handle_buffer(packets[i], entries);
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

    if (m_version < res_val[2] || (m_version == res_val[2] && m_sub_version < res_val[3])) return COMM_STATUS::PI_OLD_VERSION;

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

keys_t Protocol::update_buffer() { 
    std::vector<std::vector<uint8_t>> packets = m_serial.get_byte_vectors(END_SEQ, ESCAPE_CHAR);

    filter_latest(packets);

    return get_keys(m_buffer);
}

bool Protocol::set_sensor(uint8_t ID, uint8_t i2c_address, uint8_t interval_entry, uint8_t type) {
    if (ID == RESERVED_BUFFER_ENTRY) {
        std::cerr << "This ID is reserved" << std::endl;
        return false;
    }
    uint16_t p_sensor[2];

    p_sensor[0] = (static_cast<uint16_t>(ID) << 8) | i2c_address;
    p_sensor[1] = (static_cast<uint16_t>(interval_entry) << 8) | type;

    return send_packet(COMM_TYPE::SENSOR, p_sensor, 2) != -1;
}

packet_t Protocol::get_sensor(uint8_t ID) {
    if (ID == RESERVED_BUFFER_ENTRY) {
        std::cerr << "This ID is reserved" << std::endl;
        return {COMM_STATUS::SERIAL_NOT_IN_BUFFER, std::nullopt};
    }
    return get_packet(ID);
}

packet_t Protocol::get_heartbeat() {
    return get_packet(HB_SEQ);
}

void Protocol::disconnect() {
    m_serial.disconnect_serial();
}

Protocol::~Protocol() {
    disconnect();
}