#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include <chrono>

namespace Search {
    extern uint64_t nodes;
    extern bool time_over;
    extern std::chrono::time_point<std::chrono::steady_clock> start_time;
    extern int max_time_ms;

    void start_search(Board& board, int depth_limit, int time_limit_ms);
}

#endif
