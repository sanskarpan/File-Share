#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "Common.h"

// Protocol message structure
struct MessageHeader {
    uint32_t magic;          // Magic number for validation
    uint32_t version;        // Protocol version
    MessageType type;        // Message type
    uint32_t payload_size;   // Size of payload data
    uint32_t checksum;       // CRC32 checksum of payload
    
    static constexpr uint32_t MAGIC_NUMBER = 0x50325032; // "P2P2"
    static constexpr uint32_t PROTOCOL_VERSION = 1;
    
    MessageHeader() : magic(MAGIC_NUMBER), version(PROTOCOL_VERSION), 
                      type(MessageType::PING), payload_size(0), checksum(0) {}
    
    bool isValid() const {
        return magic == MAGIC_NUMBER && version == PROTOCOL_VERSION;
    }
} __attribute__((packed));

class Protocol {
public:
    // Message serialization/deserialization
    static std::vector<uint8_t> createMessage(MessageType type, const std::vector<uint8_t>& payload);
    static bool parseMessage(const std::vector<uint8_t>& data, MessageType& type, std::vector<uint8_t>& payload);
    
    // Specific message creators
    static std::vector<uint8_t> createPeerListRequest();
    static std::vector<uint8_t> createPeerListResponse(const std::vector<std::string>& peer_data);
    static std::vector<uint8_t> createFileListRequest(const std::string& peer_id);
    static std::vector<uint8_t> createFileListResponse(const std::vector<FileInfo>& files);
    static std::vector<uint8_t> createFileRequest(const std::string& filename, size_t offset = 0, size_t length = 0);
    static std::vector<uint8_t> createFileChunk(const std::vector<uint8_t>& chunk_data, size_t offset);
    static std::vector<uint8_t> createErrorMessage(ErrorCode code, const std::string& message);
    
    // Message parsers
    static bool parsePeerListResponse(const std::vector<uint8_t>& payload, std::vector<std::string>& peer_data);
    static bool parseFileListResponse(const std::vector<uint8_t>& payload, std::vector<FileInfo>& files);
    static bool parseFileRequest(const std::vector<uint8_t>& payload, std::string& filename, size_t& offset, size_t& length);
    static bool parseFileChunk(const std::vector<uint8_t>& payload, std::vector<uint8_t>& chunk_data, size_t& offset);
    static bool parseErrorMessage(const std::vector<uint8_t>& payload, ErrorCode& code, std::string& message);
    
    // Utility functions
    static uint32_t calculateCRC32(const std::vector<uint8_t>& data);
    static void serializeString(std::vector<uint8_t>& buffer, const std::string& str);
    static bool deserializeString(const std::vector<uint8_t>& buffer, size_t& offset, std::string& str);
    static void serializeUint32(std::vector<uint8_t>& buffer, uint32_t value);
    static bool deserializeUint32(const std::vector<uint8_t>& buffer, size_t& offset, uint32_t& value);
};

#endif
