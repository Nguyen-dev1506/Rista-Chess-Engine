#ifndef TT_H
#define TT_H

#include "types.h"
#include <vector>

const int HASH_EXACT = 0;
const int HASH_ALPHA = 1;
const int HASH_BETA = 2;

struct TTEntry {
    uint64_t hash_key;
    int depth;
    int flag;
    int score;
    Move best_move;
};

const int TT_SIZE = 1048576; // 2^20 entries

extern std::vector<TTEntry> TT;
extern const int UNKNOWN_SCORE;

void init_tt();
void clear_tt();
int probe_tt(uint64_t hash_key, int depth, int alpha, int beta, Move& best_move);
void store_tt(uint64_t hash_key, int depth, int flag, int score, Move best_move);

#endif
