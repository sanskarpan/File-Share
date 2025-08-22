#include <gtest/gtest.h>
#include "Protocol.h"

class ProtocolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test data
        test_files.emplace_back("file1.txt", "/path/file1.txt", 1024, "hash1", 1234567890);
        test_files.emplace_back("file2.jpg", "/path/file2.jpg", 2048, "hash2", 1234567891);
        
        test_peers = {"peer1|192.168.1.1|8888|1", "peer2|192.168.1.2|8889|1"};
    }
    
    std::vector<FileInfo> test_files;
    std::vector<std::string> test_peers;
};

TEST_F(ProtocolTest, MessageCreationAndParsing) {
    // Test simple message
    std::string test_payload = "Hello, P2P World!";
    std::vector<uint8_t> payload(test_payload.begin(), test_payload.end());
    
    auto message = Protocol::createMessage(MessageType::PING, payload);
    EXPECT_GT(message.size(), payload.size());  // Should include header
    
    // Parse message back
    MessageType parsed_type;
    std::vector<uint8_t> parsed_payload;
    
    EXPECT_TRUE(Protocol::parseMessage(message, parsed_type, parsed_payload));
    EXPECT_EQ(parsed_type, MessageType::PING);
    EXPECT_EQ(parsed_payload.size(), payload.size());
    EXPECT_EQ(std::string(parsed_payload.begin(), parsed_payload.end()), test_payload);
}

TEST_F(ProtocolTest, PeerListMessages) {
    // Create peer list response
    auto message = Protocol::createPeerListResponse(test_peers);
    
    MessageType type;
    std::vector<uint8_t> payload;
    EXPECT_TRUE(Protocol::parseMessage(message, type, payload));
    EXPECT_EQ(type, MessageType::PEER_LIST_RESPONSE);
    
    // Parse peer list
    std::vector<std::string> parsed_peers;
    EXPECT_TRUE(Protocol::parsePeerListResponse(payload, parsed_peers));
    EXPECT_EQ(parsed_peers.size(), test_peers.size());
    EXPECT_EQ(parsed_peers[0], test_peers[0]);
    EXPECT_EQ(parsed_peers[1], test_peers[1]);
}

TEST_F(ProtocolTest, FileListMessages) {
    // Create file list response
    auto message = Protocol::createFileListResponse(test_files);
    
    MessageType type;
    std::vector<uint8_t> payload;
    EXPECT_TRUE(Protocol::parseMessage(message, type, payload));
    EXPECT_EQ(type, MessageType::FILE_LIST_RESPONSE);
    
    // Parse file list
    std::vector<FileInfo> parsed_files;
    EXPECT_TRUE(Protocol::parseFileListResponse(payload, parsed_files));
    EXPECT_EQ(parsed_files.size(), test_files.size());
    EXPECT_EQ(parsed_files[0].filename, test_files[0].filename);
    EXPECT_EQ(parsed_files[0].size, test_files[0].size);
    EXPECT_EQ(parsed_files[0].hash, test_files[0].hash);
}

TEST_F(ProtocolTest, FileRequestMessages) {
    std::string filename = "test.txt";
    size_t offset = 1024;
    size_t length = 2048;
    
    auto message = Protocol::createFileRequest(filename, offset, length);
    
    MessageType type;
    std::vector<uint8_t> payload;
    EXPECT_TRUE(Protocol::parseMessage(message, type, payload));
    EXPECT_EQ(type, MessageType::FILE_REQUEST);
    
    std::string parsed_filename;
    size_t parsed_offset, parsed_length;
    EXPECT_TRUE(Protocol::parseFileRequest(payload, parsed_filename, parsed_offset, parsed_length));
    EXPECT_EQ(parsed_filename, filename);
    EXPECT_EQ(parsed_offset, offset);
    EXPECT_EQ(parsed_length, length);
}

TEST_F(ProtocolTest, ErrorMessages) {
    ErrorCode error_code = ErrorCode::FILE_NOT_FOUND;
    std::string error_msg = "File not found on this peer";
    
    auto message = Protocol::createErrorMessage(error_code, error_msg);
    
    MessageType type;
    std::vector<uint8_t> payload;
    EXPECT_TRUE(Protocol::parseMessage(message, type, payload));
    EXPECT_EQ(type, MessageType::ERROR_MESSAGE);
    
    ErrorCode parsed_code;
    std::string parsed_msg;
    EXPECT_TRUE(Protocol::parseErrorMessage(payload, parsed_code, parsed_msg));
    EXPECT_EQ(parsed_code, error_code);
    EXPECT_EQ(parsed_msg, error_msg);
}

TEST_F(ProtocolTest, CRC32Validation) {
    std::vector<uint8_t> test_data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    uint32_t crc1 = Protocol::calculateCRC32(test_data);
    uint32_t crc2 = Protocol::calculateCRC32(test_data);
    
    // Same data should produce same CRC
    EXPECT_EQ(crc1, crc2);
    
    // Different data should produce different CRC
    test_data.push_back(11);
    uint32_t crc3 = Protocol::calculateCRC32(test_data);
    EXPECT_NE(crc1, crc3);
}

TEST_F(ProtocolTest, InvalidMessageHandling) {
    // Test invalid header
    std::vector<uint8_t> invalid_message = {0x00, 0x01, 0x02, 0x03};
    
    MessageType type;
    std::vector<uint8_t> payload;
    EXPECT_FALSE(Protocol::parseMessage(invalid_message, type, payload));
    
    // Test corrupted message
    auto valid_message = Protocol::createMessage(MessageType::PING, {1, 2, 3, 4});
    valid_message[10] = 0xFF;  // Corrupt some byte
    
    // Should fail CRC check
    EXPECT_FALSE(Protocol::parseMessage(valid_message, type, payload));
}