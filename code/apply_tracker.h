#pragma once
#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <set>

using namespace std;

struct ApplyTracker {
    mutex              mu;
    condition_variable cv;
    uint64_t           applied_max = 0;
    set<uint64_t>      done_above;

    void MarkDone(uint64_t v) {
        lock_guard<mutex> lk(mu);
        if (v == applied_max + 1) {
            applied_max++;
            while (done_above.count(applied_max + 1)) {
                done_above.erase(applied_max + 1);
                applied_max++;
            }
            cv.notify_all();
        } else {
            done_above.insert(v);
        }
    }

    void WaitUntil(uint64_t target) {
        unique_lock<mutex> lk(mu);
        cv.wait(lk, [&]{ return applied_max >= target; });
    }
};