#ifndef __MACRO_THREAD_POOL_H__
#define __MACRO_THREAD_POOL_H__

#include <vector>
#include <queue>
#include <pthread.h>
#include "MacroAction.h" // Forward declaration would be possible, but direct include is simpler here.

/**
 * @class MacroThreadPool
 * @brief Manages a pool of worker threads for asynchronous macro execution.
 *
 * This static utility class initializes a fixed number of threads that wait
 * for tasks (macro executions) on a queue. This avoids the performance overhead
 * of creating and destroying a thread for every single macro activation.
 * It is designed to be thread-safe.
 */
class MacroThreadPool {
public:
    // This class is a static utility and should not be instantiated.
    MacroThreadPool() = delete;

    /**
     * @brief Initializes the thread pool and starts the worker threads.
     * This should be called once when the application starts.
     * @param num_threads The number of worker threads to create in the pool.
     */
    static void initialize(int num_threads);

    /**
     * @brief Shuts down the thread pool gracefully.
     * It waits for all currently executing tasks to complete and then joins all threads.
     * This should be called once when the application is exiting.
     */
    static void shutdown();

    /**
     * @brief Submits a macro to be executed by a worker thread.
     * @param action A pointer to the MacroAction object to be executed.
     */
    static void submit(MacroAction* action);

private:
    /** @brief The function that each worker thread will execute in a loop. */
    static void* worker_function(void* arg);

    /** A vector holding the thread handles for all worker threads. */
    static std::vector<pthread_t> _worker_threads;
    /** A queue to hold pending macro actions waiting for execution. */
    static std::queue<MacroAction*> _task_queue;
    /** A mutex to protect access to the task queue. */
    static pthread_mutex_t _queue_mutex;
    /** A condition variable to signal worker threads that a new task is available. */
    static pthread_cond_t _queue_cond;
    /** A flag to signal all worker threads to terminate. */
    static bool _shutdown_flag;
};

#endif // __MACRO_THREAD_POOL_H__