#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include <chrono>
#include <atomic>

namespace Search {
    extern std::atomic<uint64_t> nodes;
    extern std::atomic<bool> time_over;
    extern std::atomic<bool> own_book;
    // A/B test switches (default true = current behavior unchanged).
    extern std::atomic<bool> non_pv_check_ext;
    extern std::atomic<bool> improving_bonus_on;
    extern std::atomic<bool> singular_ext_on;
    extern std::chrono::time_point<std::chrono::steady_clock> start_time;
    extern int max_time_ms;

    void init_LMR();
    void clear_history();
    int score_move(const Board& board, Move m, uint16_t tt_move, int ply);
    int negamax(Board& board, int depth, int ply, int alpha, int beta, bool do_null, int root_depth, uint16_t excluded_move = 0);
    int quiescence(Board& board, int alpha, int beta, int ply);
    void start_search(Board board, int depth_limit, int hard_time_ms, int soft_time_ms, int num_threads = 1);
    uint64_t perft(Board& board, int depth);
}

#endif
