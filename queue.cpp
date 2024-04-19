#include <mutex>
#include <deque>
#include "semaphore.cpp"

#define Deque deque

using namespace std;

template<typename T> class FIFOQueue {

    private:
    
        Mutex     lock;
        Semaphore gate;
        Deque<T>  queue;
        int count = 0;

    public:

        void push(T toPush) {

            lock.lock();
            queue.push_back(toPush);
            count++;
            lock.unlock();
            gate.release();

        }

        T pop() {

            gate.acquire();

            T item = queue.front();

            lock.lock();
            queue.pop_front();
            count--;
            lock.unlock();

            return item;

        }

        void clear() {

            lock.lock();
            queue.clear();
            count = 0;
            gate.reset();
            lock.unlock();

        }

        int size() {
            return count;
        }

        bool hasWaiting() {
            return count > 0;
        }
        
};