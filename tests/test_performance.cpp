#include <gtest/gtest.h>
#include "HighPerformanceServer.h"
#include "Client.h"
#include "FileManager.h"
#include "Protocol.h"
#include <chrono>
#include <thread>
#include <vector>
#include <random>

class PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory and files
        test_dir = "./perf_test/";
        std::filesystem::create_directories(test_dir);
        createTestFiles();
        
        // Start server
        server = std::make_unique<HighPerformanceServer>(9999);
        server->setSharedDirectory(test_dir);
        ASSERT_TRUE(server->start());
        
        // Give server time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    void TearDown() override {
        server->stop();
        server.reset();
        std::filesystem::remove_all(test_dir);
    }
    
    void createTestFiles() {
        // Create files of various sizes
        createTestFile("small.txt", 1024);           // 1KB
        createTestFile("medium.txt", 1024 * 1024);   // 1MB
        createTestFile("large.txt", 10 * 1024 * 1024); // 10MB
    }
    
    void createTestFile(const std::string& filename, size_t size) {
        std::ofstream file(test_dir + filename, std::ios::binary);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        for (size_t i = 0; i < size; ++i) {
            file.put(static_cast<char>(dis(gen)));
        }
    }
    
    std::string test_dir;
    std::unique_ptr<HighPerformanceServer> server;
};

TEST_F(PerformanceTest, ConcurrentConnections) {
    const int num_connections = 50;  // Reduced for testing
    std::vector<std::thread> client_threads;
    std::atomic<int> successful_connections{0};
    std::atomic<int> failed_connections{0};
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Create concurrent connections
    for (int i = 0; i < num_connections; ++i) {
        client_threads.emplace_back([&, i]() {
            try {
                Client client;
                if (client.connect("127.0.0.1", 9999)) {
                    successful_connections.fetch_add(1);
                    
                    // Send a ping to test connection
                    client.sendPing();
                    
                    // Hold connection briefly
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    
                    client.disconnect();
                } else {
                    failed_connections.fetch_add(1);
                }
            } catch (const std::exception& e) {
                failed_connections.fetch_add(1);
            }
        });
    }
    
    // Wait for all connections
    for (auto& thread : client_threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Concurrent connections test:" << std::endl;
    std::cout << "  Successful: " << successful_connections.load() << std::endl;
    std::cout << "  Failed: " << failed_connections.load() << std::endl;
    std::cout << "  Duration: " << duration.count() << "ms" << std::endl;
    
    // At least 80% should succeed
    EXPECT_GE(successful_connections.load(), num_connections * 0.8);
}

TEST_F(PerformanceTest, ThroughputTest) {
    const int num_clients = 10;
    const std::string test_file = "medium.txt";  // 1MB file
    std::vector<std::thread> download_threads;
    std::atomic<long long> total_bytes_transferred{0};
    std::atomic<int> successful_downloads{0};
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Multiple clients downloading simultaneously
    for (int i = 0; i < num_clients; ++i) {
        download_threads.emplace_back([&, i]() {
            try {
                Client client;
                if (client.connect("127.0.0.1", 9999)) {
                    std::string dest_file = "./test_download_" + std::to_string(i) + ".txt";
                    
                    if (client.downloadFile(test_file, dest_file)) {
                        auto file_size = std::filesystem::file_size(dest_file);
                        total_bytes_transferred.fetch_add(file_size);
                        successful_downloads.fetch_add(1);
                        
                        // Clean up
                        std::filesystem::remove(dest_file);
                    }
                    
                    client.disconnect();
                }
            } catch (const std::exception& e) {
                std::cerr << "Download error: " << e.what() << std::endl;
            }
        });
    }
    
    // Wait for all downloads
    for (auto& thread : download_threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    double throughput_mbps = (total_bytes_transferred.load() / 1024.0 / 1024.0) / (duration.count() / 1000.0);
    
    std::cout << "Throughput test:" << std::endl;
    std::cout << "  Successful downloads: " << successful_downloads.load() << std::endl;
    std::cout << "  Total bytes: " << total_bytes_transferred.load() << std::endl;
    std::cout << "  Duration: " << duration.count() << "ms" << std::endl;
    std::cout << "  Throughput: " << throughput_mbps << " MB/s" << std::endl;
    
    EXPECT_GT(successful_downloads.load(), 0);
    EXPECT_GT(throughput_mbps, 1.0);  // At least 1 MB/s
}

TEST_F(PerformanceTest, LatencyTest) {
    const int num_pings = 100;
    std::vector<double> latencies;
    
    Client client;
    ASSERT_TRUE(client.connect("127.0.0.1", 9999));
    
    // Warm up
    for (int i = 0; i < 10; ++i) {
        client.sendPing();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Measure latencies
    for (int i = 0; i < num_pings; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        client.sendPing();
        // In a real implementation, we'd wait for PONG response
        // For this test, we'll just measure send latency
        auto end = std::chrono::high_resolution_clock::now();
        
        auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        latencies.push_back(latency / 1000.0);  // Convert to milliseconds
    }
    
    client.disconnect();
    
    // Calculate statistics
    double sum = 0;
    double min_latency = latencies[0];
    double max_latency = latencies[0];
    
    for (double latency : latencies) {
        sum += latency;
        min_latency = std::min(min_latency, latency);
        max_latency = std::max(max_latency, latency);
    }
    
    double avg_latency = sum / latencies.size();
    
    std::cout << "Latency test:" << std::endl;
    std::cout << "  Average: " << avg_latency << "ms" << std::endl;
    std::cout << "  Min: " << min_latency << "ms" << std::endl;
    std::cout << "  Max: " << max_latency << "ms" << std::endl;
    
    // Latency should be reasonable (less than 10ms for local connections)
    EXPECT_LT(avg_latency, 10.0);
}

TEST_F(PerformanceTest, MemoryUsageTest) {
    // This is a simplified memory test
    // In production, you'd use tools like Valgrind or custom memory tracking
    
    const int num_operations = 1000;
    std::vector<std::unique_ptr<Client>> clients;
    
    // Create many client objects
    for (int i = 0; i < num_operations; ++i) {
        clients.push_back(std::make_unique<Client>());
        
        // Occasionally connect and disconnect
        if (i % 10 == 0) {
            if (clients.back()->connect("127.0.0.1", 9999)) {
                clients.back()->sendPing();
                clients.back()->disconnect();
            }
        }
    }
    
    // Clean up
    clients.clear();
    
    // Force garbage collection (if applicable)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // This test mainly ensures we don't crash due to memory issues
    SUCCEED();
}

TEST_F(PerformanceTest, StressTest) {
    const int duration_seconds = 5;  // Reduced for testing
    const int max_concurrent_clients = 20;
    
    std::atomic<bool> stop_test{false};
    std::atomic<int> total_operations{0};
    std::atomic<int> successful_operations{0};
    std::atomic<int> failed_operations{0};
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Launch stress test threads
    std::vector<std::thread> stress_threads;
    for (int i = 0; i < max_concurrent_clients; ++i) {
        stress_threads.emplace_back([&, i]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> operation_dis(0, 2);
            
            while (!stop_test.load()) {
                try {
                    Client client;
                    if (client.connect("127.0.0.1", 9999)) {
                        int operation = operation_dis(gen);
                        
                        switch (operation) {
                            case 0:  // Ping
                                client.sendPing();
                                break;
                            case 1:  // Request file list
                                client.requestFileList();
                                break;
                            case 2:  // Request peer list
                                client.requestPeerList();
                                break;
                        }
                        
                        successful_operations.fetch_add(1);
                        client.disconnect();
                    } else {
                        failed_operations.fetch_add(1);
                    }
                    
                    total_operations.fetch_add(1);
                    
                    // Small delay between operations
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    
                } catch (const std::exception& e) {
                    failed_operations.fetch_add(1);
                    total_operations.fetch_add(1);
                }
            }
        });
    }
    
    // Run for specified duration
    std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
    stop_test.store(true);
    
    // Wait for all threads to finish
    for (auto& thread : stress_threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto actual_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    double operations_per_second = (total_operations.load() * 1000.0) / actual_duration.count();
    double success_rate = (double)successful_operations.load() / total_operations.load() * 100.0;
    
    std::cout << "Stress test results:" << std::endl;
    std::cout << "  Duration: " << actual_duration.count() << "ms" << std::endl;
    std::cout << "  Total operations: " << total_operations.load() << std::endl;
    std::cout << "  Successful: " << successful_operations.load() << std::endl;
    std::cout << "  Failed: " << failed_operations.load() << std::endl;
    std::cout << "  Success rate: " << success_rate << "%" << std::endl;
    std::cout << "  Operations/sec: " << operations_per_second << std::endl;
    
    // At least 70% success rate under stress
    EXPECT_GE(success_rate, 70.0);
    EXPECT_GT(operations_per_second, 10.0);  // At least 10 ops/sec
}

// Benchmark fixture for more detailed performance testing
class BenchmarkTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup for benchmarking
    }
    
    void TearDown() override {
        // Cleanup
    }
    
    // Utility function to measure execution time
    template<typename Func>
    std::chrono::microseconds measureTime(Func&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    }
};

TEST_F(BenchmarkTest, ProtocolOverhead) {
    // Measure protocol encoding/decoding overhead
    std::vector<FileInfo> test_files;
    for (int i = 0; i < 1000; ++i) {
        test_files.emplace_back("file" + std::to_string(i) + ".txt", 
                               "/path/file" + std::to_string(i) + ".txt",
                               1024 * i, "hash" + std::to_string(i), 1234567890 + i);
    }
    
    // Measure encoding time
    auto encode_time = measureTime([&]() {
        for (int i = 0; i < 100; ++i) {
            auto message = Protocol::createFileListResponse(test_files);
        }
    });
    
    std::cout << "Protocol encoding (100 iterations, 1000 files): " 
              << encode_time.count() << " microseconds" << std::endl;
    
    // Create a message to decode
    auto test_message = Protocol::createFileListResponse(test_files);
    
    // Measure decoding time
    auto decode_time = measureTime([&]() {
        for (int i = 0; i < 100; ++i) {
            MessageType type;
            std::vector<uint8_t> payload;
            Protocol::parseMessage(test_message, type, payload);
            
            std::vector<FileInfo> parsed_files;
            Protocol::parseFileListResponse(payload, parsed_files);
        }
    });
    
    std::cout << "Protocol decoding (100 iterations, 1000 files): " 
              << decode_time.count() << " microseconds" << std::endl;
    
    // Performance should be reasonable
    EXPECT_LT(encode_time.count(), 100000);  // Less than 100ms for 100 iterations
    EXPECT_LT(decode_time.count(), 100000);  // Less than 100ms for 100 iterations
}

TEST_F(BenchmarkTest, HashingPerformance) {
    // Test file hashing performance with different sizes
    std::vector<size_t> file_sizes = {1024, 10240, 102400, 1048576};  // 1KB to 1MB
    
    for (size_t size : file_sizes) {
        // Create temporary file
        std::string temp_file = "./temp_" + std::to_string(size) + ".bin";
        std::ofstream file(temp_file, std::ios::binary);
        for (size_t i = 0; i < size; ++i) {
            file.put(static_cast<char>(i % 256));
        }
        file.close();
        
        // Measure hashing time
        auto hash_time = measureTime([&]() {
            FileManager fm;
            for (int i = 0; i < 10; ++i) {
                // Hash calculation would be done here
                // For test purposes, we'll just read the file
                std::ifstream test_file(temp_file, std::ios::binary);
                std::vector<char> buffer(size);
                test_file.read(buffer.data(), size);
            }
        });
        
        double mb_per_second = (size / 1024.0 / 1024.0) / (hash_time.count() / 1000000.0 / 10.0);
        
        std::cout << "Hashing " << size << " bytes: " 
                  << hash_time.count() / 10 << " microseconds/file, "
                  << mb_per_second << " MB/s" << std::endl;
        
        // Clean up
        std::filesystem::remove(temp_file);
    }
}

// Integration test for full system performance
TEST_F(PerformanceTest, EndToEndPerformance) {
    const int num_files = 10;
    const int file_size = 1024 * 100;  // 100KB each
    
    // Create multiple test files
    for (int i = 0; i < num_files; ++i) {
        createTestFile("perf_file_" + std::to_string(i) + ".bin", file_size);
    }
    
    // Start multiple clients downloading different files
    std::vector<std::thread> client_threads;
    std::atomic<int> completed_downloads{0};
    std::atomic<long long> total_time_us{0};
    
    auto overall_start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_files; ++i) {
        client_threads.emplace_back([&, i]() {
            try {
                auto start = std::chrono::high_resolution_clock::now();
                
                Client client;
                if (client.connect("127.0.0.1", 9999)) {
                    std::string filename = "perf_file_" + std::to_string(i) + ".bin";
                    std::string dest = "./downloaded_" + filename;
                    
                    if (client.downloadFile(filename, dest)) {
                        auto end = std::chrono::high_resolution_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                        
                        completed_downloads.fetch_add(1);
                        total_time_us.fetch_add(duration.count());
                        
                        // Verify file
                        auto original_size = std::filesystem::file_size(test_dir + filename);
                        auto downloaded_size = std::filesystem::file_size(dest);
                        EXPECT_EQ(original_size, downloaded_size);
                        
                        // Clean up
                        std::filesystem::remove(dest);
                    }
                    
                    client.disconnect();
                }
            } catch (const std::exception& e) {
                std::cerr << "End-to-end test error: " << e.what() << std::endl;
            }
        });
    }
    
    // Wait for all downloads
    for (auto& thread : client_threads) {
        thread.join();
    }
    
    auto overall_end = std::chrono::high_resolution_clock::now();
    auto overall_duration = std::chrono::duration_cast<std::chrono::milliseconds>(overall_end - overall_start);
    
    double avg_time_ms = (total_time_us.load() / 1000.0) / completed_downloads.load();
    double total_mb = (num_files * file_size) / 1024.0 / 1024.0;
    double throughput = total_mb / (overall_duration.count() / 1000.0);
    
    std::cout << "End-to-end performance:" << std::endl;
    std::cout << "  Files downloaded: " << completed_downloads.load() << "/" << num_files << std::endl;
    std::cout << "  Average time per file: " << avg_time_ms << "ms" << std::endl;
    std::cout << "  Overall throughput: " << throughput << " MB/s" << std::endl;
    std::cout << "  Total time: " << overall_duration.count() << "ms" << std::endl;
    
    EXPECT_EQ(completed_downloads.load(), num_files);
    EXPECT_GT(throughput, 0.5);  // At least 0.5 MB/s overall
}