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

    public:

        void push(T toPush) {

            lock.lock();
            queue.push_back(toPush);
            lock.unlock();
            gate.release();

        }

        T pop() {

            gate.acquire();

            T item = queue.front();

            lock.lock();
            queue.pop_front();
            lock.unlock();

            return item;

        }

        void clear() {

            lock.lock();
            queue.clear();
            gate.reset();
            lock.unlock();

        }

        int size() {
            return queue.size();
        }
        
};