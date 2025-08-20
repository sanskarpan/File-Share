#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this] {
            for (;;) {
                std::function<void()> task;
                
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    condition.wait(lock, [this] { return stop.load() || !tasks.empty(); });
                    
                    if (stop.load() && tasks.empty()) {
                        return;
                    }
                    
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                
                try {
                    task();
                } catch (const std::exception& e) {
                    std::cerr << "ThreadPool task exception: " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "ThreadPool task unknown exception" << std::endl;
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::shutdown() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop.store(true);
    }
    
    condition.notify_all();
    
    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    workers.clear();
}

size_t ThreadPool::queueSize() const {
    std::lock_guard<std::mutex> lock(queue_mutex);
    return tasks.size();
}