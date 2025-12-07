/**
 * FastCollection v1.0.0 - Task Queue Example (C++)
 * 
 * Demonstrates using FastQueue for persistent task processing.
 * 
 * Copyright © 2025-2030, Ashutosh Sinha (ajsinha@gmail.com)
 * Patent Pending
 */

#include "fastcollection.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <random>
#include <ctime>
#include <thread>
#include <chrono>

using namespace fastcollection;

/**
 * Task structure with simple serialization
 */
struct Task {
    std::string id;
    std::string type;
    std::string payload;
    int priority;
    int64_t createdAt;
    int retryCount;
    int maxRetries;
    
    Task() : priority(5), createdAt(0), retryCount(0), maxRetries(3) {}
    
    Task(const std::string& id, const std::string& type, 
         const std::string& payload, int priority = 5)
        : id(id), type(type), payload(payload), priority(priority)
        , createdAt(std::time(nullptr)), retryCount(0), maxRetries(3) {}
    
    std::string serialize() const {
        std::ostringstream oss;
        oss << id << "|" << type << "|" << payload << "|" 
            << priority << "|" << createdAt << "|" 
            << retryCount << "|" << maxRetries;
        return oss.str();
    }
    
    static Task deserialize(const std::string& data) {
        Task task;
        std::istringstream iss(data);
        std::string token;
        
        std::getline(iss, task.id, '|');
        std::getline(iss, task.type, '|');
        std::getline(iss, task.payload, '|');
        
        std::getline(iss, token, '|');
        task.priority = std::stoi(token);
        
        std::getline(iss, token, '|');
        task.createdAt = std::stoll(token);
        
        std::getline(iss, token, '|');
        task.retryCount = std::stoi(token);
        
        std::getline(iss, token, '|');
        task.maxRetries = std::stoi(token);
        
        return task;
    }
    
    bool shouldRetry() const {
        return retryCount < maxRetries;
    }
    
    std::string toString() const {
        std::ostringstream oss;
        oss << "Task{id=" << id << ", type=" << type 
            << ", priority=" << priority 
            << ", retries=" << retryCount << "/" << maxRetries << "}";
        return oss.str();
    }
};

/**
 * Task queue with dead letter queue support
 */
class TaskQueue {
private:
    FastQueue mainQueue;
    FastQueue deadLetterQueue;
    int taskTTL;
    
    void submitInternal(const Task& task, bool front) {
        std::string data = task.serialize();
        if (front) {
            mainQueue.offerFirst(
                reinterpret_cast<const uint8_t*>(data.data()), 
                data.size(), 
                taskTTL
            );
        } else {
            mainQueue.offer(
                reinterpret_cast<const uint8_t*>(data.data()), 
                data.size(), 
                taskTTL
            );
        }
    }
    
public:
    TaskQueue(const std::string& basePath, int ttlSeconds = 3600)
        : mainQueue(basePath + "/tasks.fc", 64 * 1024 * 1024, true)
        , deadLetterQueue(basePath + "/dlq.fc", 16 * 1024 * 1024, true)
        , taskTTL(ttlSeconds) {}
    
    void submit(const Task& task) {
        if (task.priority == 0) {
            // High priority - add to front
            submitInternal(task, true);
        } else {
            submitInternal(task, false);
        }
        std::cout << "Submitted: " << task.toString() << std::endl;
    }
    
    bool poll(Task& task) {
        std::vector<uint8_t> result;
        if (!mainQueue.poll(result)) {
            return false;
        }
        
        std::string data(result.begin(), result.end());
        task = Task::deserialize(data);
        return true;
    }
    
    bool peek(Task& task) {
        std::vector<uint8_t> result;
        if (!mainQueue.peek(result)) {
            return false;
        }
        
        std::string data(result.begin(), result.end());
        task = Task::deserialize(data);
        return true;
    }
    
    void requeue(Task& task) {
        task.retryCount++;
        
        if (task.shouldRetry()) {
            std::cout << "  Requeuing: " << task.id 
                      << " (attempt " << task.retryCount 
                      << "/" << task.maxRetries << ")" << std::endl;
            submitInternal(task, false);
        } else {
            std::cout << "  Moving to DLQ: " << task.id 
                      << " (max retries exceeded)" << std::endl;
            std::string data = task.serialize();
            deadLetterQueue.offer(
                reinterpret_cast<const uint8_t*>(data.data()),
                data.size(),
                -1  // Keep forever in DLQ
            );
        }
    }
    
    size_t size() const {
        return mainQueue.size();
    }
    
    size_t dlqSize() const {
        return deadLetterQueue.size();
    }
};

/**
 * Simulate task processing with random failures
 */
bool processTask(const Task& task) {
    std::cout << "Processing: " << task.toString() << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 30% chance of failure
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1, 10);
    
    if (dis(gen) <= 3) {
        std::cout << "  FAILED!" << std::endl;
        return false;
    }
    
    std::cout << "  SUCCESS!" << std::endl;
    return true;
}

int main() {
    std::cout << "FastCollection v1.0.0 - Task Queue Example (C++)" << std::endl;
    std::cout << std::string(55, '=') << std::endl;
    std::cout << std::endl;
    
    try {
        TaskQueue taskQueue("/tmp/taskqueue_cpp", 3600);
        
        // Submit various tasks
        std::cout << "Submitting tasks..." << std::endl << std::endl;
        
        taskQueue.submit(Task("t1", "EMAIL", "Send welcome email", 2));
        taskQueue.submit(Task("t2", "REPORT", "Generate report", 3));
        taskQueue.submit(Task("t3", "ALERT", "Critical alert!", 0));  // High priority
        taskQueue.submit(Task("t4", "BACKUP", "Backup database", 5));
        taskQueue.submit(Task("t5", "NOTIFY", "Push notification", 1));
        
        std::cout << std::endl << "Queue size: " << taskQueue.size() << std::endl;
        
        Task nextTask;
        if (taskQueue.peek(nextTask)) {
            std::cout << "Next task: " << nextTask.toString() << std::endl;
        }
        
        // Process tasks
        std::cout << std::endl << "--- Processing Tasks ---" << std::endl << std::endl;
        
        Task task;
        while (taskQueue.poll(task)) {
            if (!processTask(task)) {
                taskQueue.requeue(task);
            }
        }
        
        std::cout << std::endl << "--- Summary ---" << std::endl;
        std::cout << "Queue size: " << taskQueue.size() << std::endl;
        std::cout << "Dead letter queue size: " << taskQueue.dlqSize() << std::endl;
        
        std::cout << std::endl << "Example completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
