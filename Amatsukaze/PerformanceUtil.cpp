/**
* Amtasukaze Avisynth Source Plugin
* Copyright (c) 2017-2019 Nekopanda
*
* This software is released under the MIT License.
* http://opensource.org/licenses/mit-license.php
*/

#include "PerformanceUtil.h"

#ifdef _WIN32
Stopwatch::Stopwatch()
    : sum(0) {
    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
}

void Stopwatch::start() {
    QueryPerformanceCounter((LARGE_INTEGER*)&prev);
}

double Stopwatch::current() {
    int64_t cur;
    QueryPerformanceCounter((LARGE_INTEGER*)&cur);
    return (double)(cur - prev) / freq;
}

void Stopwatch::stop() {
    int64_t cur;
    QueryPerformanceCounter((LARGE_INTEGER*)&cur);
    sum += cur - prev;
    prev = cur;
}
#else
Stopwatch::Stopwatch()
    : sum(0) {
    
}
timespec t;
void Stopwatch::start() {
    clock_gettime(CLOCK_MONOTONIC_RAW, &prev);
}

double diff_ts(const timespec& cur, const timespec& prev) {
    return (cur.tv_sec - prev.tv_sec) + (cur.tv_nsec - prev.tv_nsec) / 1000000000;
}

double Stopwatch::current() {
    timespec cur;
    clock_gettime(CLOCK_MONOTONIC_RAW, &cur);
    return diff_ts(cur, prev);
}

void Stopwatch::stop() {
    timespec cur;
    clock_gettime(CLOCK_MONOTONIC_RAW, &cur);

    sum += diff_ts(cur, prev);
    prev = cur;
}
#endif

void Stopwatch::reset() {
    sum = 0;
}

double Stopwatch::getTotal() const {
    return sum;
}

double Stopwatch::getAndReset() {
    stop();
    double ret = getTotal();
    sum = 0;
    return ret;
}

void FpsPrinter::updateProgress(bool last) {
    sum.span += current.span;
    sum.count += current.count;
    times.push_back(current);
    current = TimeCount();

    if (last) {
        ctx.infoF("complete. %.2ffps (%dフレーム)", sum.count / sum.span, total);
    } else {
        float sumtime = 0;
        int sumcount = 0;
        for (int i = 0; i < (int)times.size(); ++i) {
            sumtime += times[i].span;
            sumcount += times[i].count;
        }
        ctx.progressF("%d/%d %.2ffps", sum.count, total, sumcount / sumtime);

        if ((int)times.size() > navg) {
            times.pop_front();
        }
    }
}
FpsPrinter::FpsPrinter(AMTContext& ctx, int navg)
    : AMTObject(ctx)
    , navg(navg) {}

void FpsPrinter::start(int total_) {
    total = total_;
    sum = TimeCount();
    current = TimeCount();
    times.clear();

    sw.start();
}

void FpsPrinter::update(int count) {
    current.count += count;
    current.span += (float)sw.getAndReset();
    if (current.span >= 0.5f) {
        updateProgress(false);
    }
}

void FpsPrinter::stop() {
    current.span += (float)sw.getAndReset();
    updateProgress(true);
    sw.stop();
}
