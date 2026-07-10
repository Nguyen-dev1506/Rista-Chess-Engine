#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include <chrono>
#include <atomic>

namespace Search {
    extern std::atomic<uint64_t> nodes;
    extern std::atomic<bool> time_over;
    extern std::chrono::time_point<std::chrono::steady_clock> start_time;
    extern int max_time_ms;

    void start_search(Board& board, int depth_limit, int time_limit_ms, int num_threads = 1);
    uint64_t perft(Board& board, int depth);
}

#endif
