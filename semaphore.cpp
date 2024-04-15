#include <mutex>
#include <condition_variable>

#define Mutex std::mutex
#define ConditionVariable std::condition_variable
#define LockGuard std::lock_guard
#define UniqueLock std::unique_lock

class Semaphore {

    Mutex             mutex;
    ConditionVariable condition;
    
    unsigned long count = 0;

public:

    void release() {

        LockGuard<decltype(mutex)> lock(mutex);

        ++count;

        condition.notify_one();
        
    }

    void acquire() {

        UniqueLock<decltype(mutex)> lock(mutex);

        while (!count) // Handle spurious wake-ups.
            condition.wait(lock);

        --count;

    }

    bool try_acquire() {

        LockGuard<decltype(mutex)> lock(mutex);

        if (count) {

            --count;
            return true;

        }

        return false;

    }

    void reset() {
        count = 0;
    }
};