#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

using namespace std;

template <typename T>
struct ConcurrentQueue {
    void Push(T val) {
        {
            lock_guard<mutex> lk(mu);
            q.push(move(val));
        }
        cv.notify_one();
    }

    // blocks while empty and open; returns nullopt when closed and drained
    optional<T> Pop() {
        unique_lock<mutex> lk(mu);
        cv.wait(lk, [this]{ return !q.empty() || closed; });
        if (q.empty()) return nullopt;
        T val = move(q.front());
        q.pop();
        return val;
    }

    // wake all blocked consumers so each can see the closed state
    void Close() {
        {
            lock_guard<mutex> lk(mu);
            closed = true;
        }
        cv.notify_all();
    }

private:
    queue<T>                q;
    mutex                   mu;
    condition_variable      cv;
    bool                    closed = false;
};