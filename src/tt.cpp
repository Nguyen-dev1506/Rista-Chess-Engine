#include "tt.h"
#include <algorithm>

TranspositionTable TT(16); // Default 16MB

TranspositionTable::TranspositionTable(size_t size_mb) {
    resize(size_mb);
}

void TranspositionTable::resize(size_t size_mb) {
    size_t target_entries = (size_mb * 1024 * 1024) / sizeof(TTEntry);
    size_t num_entries = 1;
    while (num_entries <= target_entries) num_entries *= 2;
    num_entries /= 2;
    if (num_entries < 1) num_entries = 1;
    
    mask = num_entries - 1;
    table.resize(num_entries);
    clear();
}

void TranspositionTable::clear() {
    for (auto& entry : table) {
        entry.key = 0;
        entry.depth = 0;
        entry.score = 0;
        entry.eval = TT_NO_EVAL;
        entry.flag_age = TT_EXACT;
        entry.best_move = 0;
    }
    current_age = 0;
}

bool TranspositionTable::probe(U64 key, int depth, int alpha, int beta, int& score, uint16_t& best_move, int& tt_eval, int ply) {
    tt_eval = TT_NO_EVAL;
    if (table.empty()) return false;
    TTEntry& entry = table[key & mask];

    if (entry.key == key) {
        best_move = entry.best_move;
        tt_eval = entry.eval;
        if (entry.depth >= depth) {
            TTFlag flag = static_cast<TTFlag>(entry.flag_age & 3);
            int tt_score = entry.score;
            if (tt_score >= 29900) tt_score -= ply;
            else if (tt_score <= -29900) tt_score += ply;
            
            if (flag == TT_EXACT) {
                score = tt_score;
                return true;
            }
            if (flag == TT_ALPHA && tt_score <= alpha) {
                score = tt_score;
                return true;
            }
            if (flag == TT_BETA && tt_score >= beta) {
                score = tt_score;
                return true;
            }
        }
    }
    return false;
}

bool TranspositionTable::probe_for_singular(U64 key, int& tt_depth, int& tt_score, TTFlag& tt_flag, int ply) {
    if (table.empty()) return false;
    TTEntry& entry = table[key & mask];
    if (entry.key == key) {
        tt_depth = entry.depth;
        tt_flag = static_cast<TTFlag>(entry.flag_age & 3);
        int score = entry.score;
        if (score >= 29900) score -= ply;
        else if (score <= -29900) score += ply;
        tt_score = score;
        return true;
    }
    return false;
}

void TranspositionTable::store(U64 key, int depth, int score, TTFlag flag, uint16_t best_move, int eval) {
    if (table.empty()) return;
    TTEntry& entry = table[key & mask];

    uint8_t entry_age = entry.flag_age >> 2;
    int16_t clamped_eval = static_cast<int16_t>(std::clamp(eval, -32767, 32767));

    // Replacement scheme
    if (entry.key == key) {
        if (entry.depth > depth) {
            // Keep the deeper entry, but maybe update age/best_move/eval snapshot
            if (best_move != 0) entry.best_move = best_move;
            entry.flag_age = (current_age << 2) | (entry.flag_age & 3);
            entry.eval = clamped_eval;
            return;
        }
    } else {
        if (entry_age == current_age && entry.depth > depth) {
            return;
        }
    }

    int clamped_depth = depth;
    if (clamped_depth < 0) clamped_depth = 0;
    else if (clamped_depth > 127) clamped_depth = 127;

    entry.key = key;
    entry.score = score;
    entry.eval = clamped_eval;
    entry.depth = clamped_depth;
    entry.flag_age = (current_age << 2) | (flag & 3);
    entry.best_move = best_move;
}

uint16_t TranspositionTable::probe_move(U64 key) {
    if (table.empty()) return 0;
    TTEntry& entry = table[key & mask];
    if (entry.key == key) {
        return entry.best_move;
    }
    return 0;
}
