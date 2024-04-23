#include <mutex>
#include <deque>
#include "semaphore.cpp"

using namespace std;

template<typename T> class FIFOQueue {

    private:
    
        mutex     lock;
        Semaphore gate;
        deque<T>  queue;
        int count = 0;

    public:

        void push(T toPush) {

            lock_guard<mutex> guard(lock);

            queue.push_back(toPush);
            count++;
            gate.release();

        }

        T pop() {

            gate.acquire();

            T item = queue.front();

            lock_guard<mutex> guard(lock);

            queue.pop_front();
            count--;

            return item;

        }

        void clear() {

            lock_guard<mutex> guard(lock);
            
            queue.clear();
            count = 0;
            gate.reset();
            

        }

        int size() {
            return count;
        }

        bool hasWaiting() {
            return count > 0;
        }
        
};