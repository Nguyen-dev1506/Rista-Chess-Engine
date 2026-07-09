#include "tt.h"

TranspositionTable TT(16); // Default 16MB

TranspositionTable::TranspositionTable(size_t size_mb) {
    resize(size_mb);
}

void TranspositionTable::resize(size_t size_mb) {
    size_t num_entries = (size_mb * 1024 * 1024) / sizeof(TTEntry);
    table.resize(num_entries);
    clear();
}

void TranspositionTable::clear() {
    for (auto& entry : table) {
        entry.key = 0;
        entry.depth = 0;
        entry.score = 0;
        entry.flag_age = TT_EXACT;
        entry.best_move = 0;
    }
    current_age = 0;
}

bool TranspositionTable::probe(U64 key, int depth, int alpha, int beta, int& score, uint16_t& best_move, int ply) {
    if (table.empty()) return false;
    TTEntry& entry = table[key % table.size()];
    
    if (entry.key == key) {
        best_move = entry.best_move;
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

void TranspositionTable::store(U64 key, int depth, int score, TTFlag flag, uint16_t best_move) {
    if (table.empty()) return;
    TTEntry& entry = table[key % table.size()];
    
    uint8_t entry_age = entry.flag_age >> 2;
    
    // Replacement scheme
    if (entry.key == key) {
        if (entry.depth > depth) {
            // Keep the deeper entry, but maybe update age and best_move if we have a new one
            if (best_move != 0) entry.best_move = best_move;
            entry.flag_age = (current_age << 2) | (entry.flag_age & 3);
            return;
        }
    } else {
        if (entry_age == current_age && entry.depth > depth) {
            return;
        }
    }
    
    entry.key = key;
    entry.score = score;
    entry.depth = depth;
    entry.flag_age = (current_age << 2) | (flag & 3);
    entry.best_move = best_move;
}

uint16_t TranspositionTable::probe_move(U64 key) {
    if (table.empty()) return 0;
    TTEntry& entry = table[key % table.size()];
    if (entry.key == key) {
        return entry.best_move;
    }
    return 0;
}
