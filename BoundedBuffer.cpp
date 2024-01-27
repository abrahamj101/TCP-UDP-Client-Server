// your PA3 BoundedBuffer.cpp code here
#include "BoundedBuffer.h"
#include <cassert>
#include <mutex>
#include <condition_variable>

using namespace std;

	mutex mtx;                                
	condition_variable not_full;           
	condition_variable not_empty;         

BoundedBuffer::BoundedBuffer (int _cap) : cap(_cap) {
    // modify as needed
}

BoundedBuffer::~BoundedBuffer () {
    // modify as needed
}

void BoundedBuffer::push (char* msg, int size) {
    // 1. Convert the incoming byte sequence given by msg and size into a vector<char>
    // 2. Wait until there is room in the queue (i.e., queue lengh is less than cap)
    // 3. Then push the vector at the end of the queue
    // 4. Wake up threads that were waiting for push

    //converts msg into a vector<char>
    vector<char> data(msg, msg + size);
    unique_lock<mutex> lock(mtx);
    //wait till queue has room
    not_full.wait(lock, [this]() { return static_cast<int>(q.size()) < cap; });
    //push the vector at the end of  queue
    q.push(data);
    //wake up threads that were waiting 
    not_empty.notify_one();
}

int BoundedBuffer::pop (char* msg, int size) {
    // 1. Wait until the queue has at least 1 item
    // 2. Pop the front item of the queue. The popped item is a vector<char>
    // 3. Convert the popped vector<char> into a char*, copy that into msg; assert that the vector<char>'s length is <= size
    // 4. Wake up threads that were waiting for pop
    // 5. Return the vector's length to the caller so that they know how many bytes were popped

    unique_lock<mutex> lock(mtx); 
    //wait till the queue has at least 1 item
    not_empty.wait(lock, [this]() { return !q.empty(); });

    vector<char> front_item = q.front();
    q.pop();
    //convert the pop to a char*
    assert(front_item.size() <= static_cast<size_t>(size));
    for (size_t i = 0; i < front_item.size(); i++) {
        msg[i] = front_item[i];
    }
    //wakes waiting threads
    not_full.notify_one();

    return front_item.size();
}

size_t BoundedBuffer::size () {
    lock_guard<mutex> lock(mtx);
    return q.size();
}
