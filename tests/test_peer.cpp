#include <gtest/gtest.h>
#include "Peer.h"
#include <thread>
#include <chrono>

class PeerTest : public ::testing::Test {
protected:
    void SetUp() override {
        peer = std::make_unique<Peer>("test-peer-1", "192.168.1.100", 8888);
    }
    
    void TearDown() override {
        peer.reset();
    }
    
    std::unique_ptr<Peer> peer;
};

TEST_F(PeerTest, BasicConstruction) {
    EXPECT_EQ(peer->getId(), "test-peer-1");
    EXPECT_EQ(peer->getIpAddress(), "192.168.1.100");
    EXPECT_EQ(peer->getPort(), 8888);
    EXPECT_TRUE(peer->isActive());
    EXPECT_EQ(peer->getAddress(), "192.168.1.100:8888");
}

TEST_F(PeerTest, FileManagement) {
    // Test adding files
    FileInfo file1("test.txt", "/path/test.txt", 1024, "hash1", 1234567890);
    FileInfo file2("image.jpg", "/path/image.jpg", 2048, "hash2", 1234567891);
    
    peer->addFile(file1);
    peer->addFile(file2);
    
    EXPECT_TRUE(peer->hasFile("test.txt"));
    EXPECT_TRUE(peer->hasFile("image.jpg"));
    EXPECT_FALSE(peer->hasFile("nonexistent.txt"));
    
    auto files = peer->getFiles();
    EXPECT_EQ(files.size(), 2);
    
    // Test file info retrieval
    auto retrieved_file = peer->getFileInfo("test.txt");
    EXPECT_EQ(retrieved_file.filename, "test.txt");
    EXPECT_EQ(retrieved_file.size, 1024);
    EXPECT_EQ(retrieved_file.hash, "hash1");
    
    // Test file removal
    peer->removeFile("test.txt");
    EXPECT_FALSE(peer->hasFile("test.txt"));
    EXPECT_TRUE(peer->hasFile("image.jpg"));
    
    files = peer->getFiles();
    EXPECT_EQ(files.size(), 1);
}

TEST_F(PeerTest, StatusManagement) {
    // Test initial state
    EXPECT_TRUE(peer->isActive());
    
    // Test deactivation
    peer->setActive(false);
    EXPECT_FALSE(peer->isActive());
    
    // Test reactivation
    peer->setActive(true);
    EXPECT_TRUE(peer->isActive());
}

TEST_F(PeerTest, LastSeenTracking) {
    auto initial_time = peer->getLastSeen();
    
    // Wait a bit and update
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    peer->updateLastSeen();
    
    auto updated_time = peer->getLastSeen();
    EXPECT_GT(updated_time, initial_time);
}

TEST_F(PeerTest, Serialization) {
    // Add some files
    FileInfo file1("test.txt", "/path/test.txt", 1024, "hash1", 1234567890);
    FileInfo file2("image.jpg", "/path/image.jpg", 2048, "hash2", 1234567891);
    peer->addFile(file1);
    peer->addFile(file2);
    
    // Serialize
    std::string serialized = peer->serialize();
    EXPECT_FALSE(serialized.empty());
    
    // Deserialize
    Peer deserialized_peer = Peer::deserialize(serialized);
    
    EXPECT_EQ(deserialized_peer.getId(), peer->getId());
    EXPECT_EQ(deserialized_peer.getIpAddress(), peer->getIpAddress());
    EXPECT_EQ(deserialized_peer.getPort(), peer->getPort());
    EXPECT_EQ(deserialized_peer.getFiles().size(), peer->getFiles().size());
}

TEST_F(PeerTest, ThreadSafety) {
    const int num_threads = 10;
    const int files_per_thread = 100;
    std::vector<std::thread> threads;
    
    // Multiple threads adding files
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, files_per_thread]() {
            for (int i = 0; i < files_per_thread; ++i) {
                std::string filename = "file_" + std::to_string(t) + "_" + std::to_string(i) + ".txt";
                FileInfo file(filename, "/path/" + filename, 1024, "hash_" + std::to_string(i), 1234567890);
                peer->addFile(file);
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Check total files
    auto files = peer->getFiles();
    EXPECT_EQ(files.size(), num_threads * files_per_thread);
}
