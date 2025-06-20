#include "MacroThreadPool.h"
#include <iostream>

// Initialization of static class members
std::vector<pthread_t> MacroThreadPool::_worker_threads;
std::queue<MacroAction*> MacroThreadPool::_task_queue;
pthread_mutex_t MacroThreadPool::_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t MacroThreadPool::_queue_cond = PTHREAD_COND_INITIALIZER;
bool MacroThreadPool::_shutdown_flag = false;

void MacroThreadPool::initialize(int num_threads) {
    _shutdown_flag = false;
    _worker_threads.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        pthread_t thread;
        if (pthread_create(&thread, nullptr, &MacroThreadPool::worker_function, nullptr) != 0) {
            std::cerr << "Error creating a worker thread for the MacroThreadPool." << std::endl;
        }
        _worker_threads.push_back(thread);
    }
    std::cout << "MacroThreadPool initialized with " << num_threads << " threads." << std::endl;
}

void MacroThreadPool::shutdown() {
    pthread_mutex_lock(&_queue_mutex);
    _shutdown_flag = true;
    pthread_mutex_unlock(&_queue_mutex);

    // Wake up all waiting threads so they can notice the shutdown flag.
    pthread_cond_broadcast(&_queue_cond);

    // Wait for all threads to finish their work and exit.
    for (pthread_t& thread : _worker_threads) {
        pthread_join(thread, nullptr);
    }
    _worker_threads.clear();
    std::cout << "MacroThreadPool shut down." << std::endl;
}

void MacroThreadPool::submit(MacroAction* action) {
    pthread_mutex_lock(&_queue_mutex);
    _task_queue.push(action);
    // Signal that a new task is available.
    pthread_cond_signal(&_queue_cond);
    pthread_mutex_unlock(&_queue_mutex);
}

void* MacroThreadPool::worker_function(void* arg) {
    while (true) {
        MacroAction* task = nullptr;

        pthread_mutex_lock(&_queue_mutex);
        // Wait until a task is in the queue OR the pool is shutting down.
        while (_task_queue.empty() && !_shutdown_flag) {
            pthread_cond_wait(&_queue_cond, &_queue_mutex);
        }

        // If the pool is shutting down and there are no more tasks, exit the thread.
        if (_shutdown_flag && _task_queue.empty()) {
            pthread_mutex_unlock(&_queue_mutex);
            break;
        }

        // Get a task from the queue.
        if (!_task_queue.empty()) {
            task = _task_queue.front();
            _task_queue.pop();
        }
        pthread_mutex_unlock(&_queue_mutex);

        if (task != nullptr) {
            // Execute the macro's main loop.
            task->execute_macro_loop();
        }
    }
    return nullptr;
}