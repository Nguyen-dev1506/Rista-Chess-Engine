#include "tt.h"

std::vector<TTEntry> TT;
extern const int UNKNOWN_SCORE = -30001;

void init_tt() {
    TT.resize(TT_SIZE);
    clear_tt();
}

void clear_tt() {
    for (int i = 0; i < TT_SIZE; i++) {
        TT[i].hash_key = 0;
        TT[i].depth = 0;
        TT[i].flag = 0;
        TT[i].score = 0;
        TT[i].best_move = 0;
    }
}

int probe_tt(uint64_t hash_key, int depth, int alpha, int beta, Move& best_move) {
    TTEntry& entry = TT[hash_key & (TT_SIZE - 1)];
    if (entry.hash_key == hash_key) {
        best_move = entry.best_move;
        if (entry.depth >= depth) {
            if (entry.flag == HASH_EXACT) return entry.score;
            if (entry.flag == HASH_ALPHA && entry.score <= alpha) return alpha;
            if (entry.flag == HASH_BETA && entry.score >= beta) return beta;
        }
    }
    return UNKNOWN_SCORE;
}

void store_tt(uint64_t hash_key, int depth, int flag, int score, Move best_move) {
    TTEntry& entry = TT[hash_key & (TT_SIZE - 1)];
    // Always replace strategy for now
    entry.hash_key = hash_key;
    entry.depth = depth;
    entry.flag = flag;
    entry.score = score;
    entry.best_move = best_move;
}
