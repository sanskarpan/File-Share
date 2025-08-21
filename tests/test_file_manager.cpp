#include <gtest/gtest.h>
#include "FileManager.h"
#include <filesystem>
#include <fstream>

class FileManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = "./test_shared/";
        std::filesystem::create_directories(test_dir);
        
        file_manager = std::make_unique<FileManager>();
        file_manager->setSharedDirectory(test_dir);
        
        createTestFiles();
    }
    
    void TearDown() override {
        file_manager.reset();
        std::filesystem::remove_all(test_dir);
    }
    
    void createTestFiles() {
        // Create test files
        std::ofstream file1(test_dir + "test1.txt");
        file1 << "This is test file 1 content";
        file1.close();
        
        std::ofstream file2(test_dir + "test2.txt");
        file2 << "This is test file 2 with different content";
        file2.close();
        
        std::ofstream file3(test_dir + "binary.bin");
        for (int i = 0; i < 1000; ++i) {
            file3.put(static_cast<char>(i % 256));
        }
        file3.close();
    }
    
    std::string test_dir;
    std::unique_ptr<FileManager> file_manager;
};

TEST_F(FileManagerTest, DirectoryScanning) {
    file_manager->refreshFileList();
    auto files = file_manager->getFileList();
    
    EXPECT_GE(files.size(), 3);  // At least our test files
    
    // Check specific files exist
    bool found_test1 = false, found_test2 = false, found_binary = false;
    for (const auto& file : files) {
        if (file.filename == "test1.txt") found_test1 = true;
        if (file.filename == "test2.txt") found_test2 = true;
        if (file.filename == "binary.bin") found_binary = true;
    }
    
    EXPECT_TRUE(found_test1);
    EXPECT_TRUE(found_test2);
    EXPECT_TRUE(found_binary);
}

TEST_F(FileManagerTest, FileHashing) {
    file_manager->refreshFileList();
    
    EXPECT_TRUE(file_manager->hasFile("test1.txt"));
    EXPECT_TRUE(file_manager->hasFile("test2.txt"));
    EXPECT_FALSE(file_manager->hasFile("nonexistent.txt"));
    
    auto file1_info = file_manager->getFileInfo("test1.txt");
    auto file2_info = file_manager->getFileInfo("test2.txt");
    
    // Files with different content should have different hashes
    EXPECT_NE(file1_info.hash, file2_info.hash);
    EXPECT_FALSE(file1_info.hash.empty());
    EXPECT_FALSE(file2_info.hash.empty());
}

TEST_F(FileManagerTest, FileIntegrityValidation) {
    file_manager->refreshFileList();
    auto file_info = file_manager->getFileInfo("test1.txt");
    
    // Valid hash should pass
    EXPECT_TRUE(file_manager->validateFileIntegrity(file_info.filepath, file_info.hash));
    
    // Invalid hash should fail
    EXPECT_FALSE(file_manager->validateFileIntegrity(file_info.filepath, "invalid_hash"));
}

TEST_F(FileManagerTest, FileSizes) {
    file_manager->refreshFileList();
    
    auto test1_info = file_manager->getFileInfo("test1.txt");
    auto binary_info = file_manager->getFileInfo("binary.bin");
    
    EXPECT_GT(test1_info.size, 0);
    EXPECT_EQ(binary_info.size, 1000);  // We wrote 1000 bytes
    
    // Test file size utility
    EXPECT_EQ(file_manager->getFileSize(test1_info.filepath), test1_info.size);
}