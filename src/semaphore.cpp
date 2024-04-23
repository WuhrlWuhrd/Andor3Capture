#include <mutex>
#include <condition_variable>

using namespace std;


class Semaphore {

    mutex              mtx;
    condition_variable condition;
    
    unsigned long count = 0;

public:

    void release() {

        lock_guard<decltype(mtx)> lock(mtx);

        ++count;

        condition.notify_one();
        
    }

    void acquire() {

        unique_lock<decltype(mtx)> lock(mtx);

        while (!count) // Handle spurious wake-ups.
            condition.wait(lock);

        --count;

    }

    bool try_acquire() {

        lock_guard<decltype(mtx)> lock(mtx);

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