#ifndef TT_H
#define TT_H

#include "types.h"
#include <vector>

enum TTFlag {
    TT_EXACT,
    TT_ALPHA,
    TT_BETA
};

struct alignas(16) TTEntry {
    U64 key;            // 8 bytes
    int16_t score;      // 2 bytes
    int16_t eval;       // 2 bytes: static eval snapshot (TT_NO_EVAL if none)
    uint16_t best_move; // 2 bytes
    int8_t depth;       // 1 byte
    uint8_t flag_age;   // 1 byte (2 bits for flag, 6 bits for age)
};
// Still exactly 16 bytes (8+2+2+2+1+1), no extra padding introduced --
// 4 entries per 64-byte cache line, same as before this field was added.

constexpr int16_t TT_NO_EVAL = -32768; // sentinel: no cached static eval for this entry

class TranspositionTable {
public:
    TranspositionTable(size_t size_mb);

    void resize(size_t size_mb);
    void clear();
    void new_search() { current_age = (current_age + 1) & 63; }

    bool probe(U64 key, int depth, int alpha, int beta, int& score, uint16_t& best_move, int& tt_eval, int ply);
    bool probe_for_singular(U64 key, int& tt_depth, int& tt_score, TTFlag& tt_flag, int ply);
    void store(U64 key, int depth, int score, TTFlag flag, uint16_t best_move, int eval);
    uint16_t probe_move(U64 key);
    
    inline void prefetch(U64 key) const {
        if (!table.empty()) {
            __builtin_prefetch(&table[key & mask]);
        }
    }

    size_t size() const { return table.size(); }

private:
    std::vector<TTEntry> table;
    size_t mask = 0;
    uint8_t current_age = 0;
};

extern TranspositionTable TT;

#endif
