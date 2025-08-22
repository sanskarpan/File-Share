#include <gtest/gtest.h>
#include "ThreadPool.h"
#include <atomic>
#include <chrono>

class ThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool = std::make_unique<ThreadPool>(4);
    }
    
    void TearDown() override {
        pool.reset();
    }
    
    std::unique_ptr<ThreadPool> pool;
};

TEST_F(ThreadPoolTest, BasicTaskExecution) {
    std::atomic<int> counter{0};
    
    // Submit some tasks
    for (int i = 0; i < 10; ++i) {
        pool->enqueue([&counter]() {
            counter.fetch_add(1);
        });
    }
    
    // Wait a bit for tasks to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_EQ(counter.load(), 10);
}

TEST_F(ThreadPoolTest, TaskReturnValues) {
    // Submit task that returns a value
    auto future = pool->enqueue([]() -> int {
        return 42;
    });
    
    EXPECT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, ConcurrentExecution) {
    const int num_tasks = 100;
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;
    
    // Submit many tasks
    for (int i = 0; i < num_tasks; ++i) {
        futures.push_back(pool->enqueue([&counter, i]() {
            // Simulate some work
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            counter.fetch_add(1);
        }));
    }
    
    // Wait for all tasks
    for (auto& future : futures) {
        future.get();
    }
    
    EXPECT_EQ(counter.load(), num_tasks);
}

TEST_F(ThreadPoolTest, ExceptionHandling) {
    auto future = pool->enqueue([]() -> int {
        throw std::runtime_error("Test exception");
        return 42;
    });
    
    EXPECT_THROW(future.get(), std::runtime_error);
}