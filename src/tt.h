#ifndef TT_H
#define TT_H

#include "types.h"
#include <vector>

enum TTFlag {
    TT_EXACT,
    TT_ALPHA,
    TT_BETA
};

struct TTEntry {
    U64 key;            // 8 bytes
    int16_t score;      // 2 bytes
    uint16_t best_move; // 2 bytes
    int8_t depth;       // 1 byte
    uint8_t flag_age;   // 1 byte (2 bits for flag, 6 bits for age)
};

class TranspositionTable {
public:
    TranspositionTable(size_t size_mb);
    
    void resize(size_t size_mb);
    void clear();
    void new_search() { current_age = (current_age + 1) & 63; }
    
    bool probe(U64 key, int depth, int alpha, int beta, int& score, uint16_t& best_move);
    void store(U64 key, int depth, int score, TTFlag flag, uint16_t best_move);
    uint16_t probe_move(U64 key);

    size_t size() const { return table.size(); }

private:
    std::vector<TTEntry> table;
    uint8_t current_age = 0;
};

extern TranspositionTable TT;

#endif
