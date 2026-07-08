#include "zobrist.h"
#include <random>

uint64_t piece_keys[13][120];
uint64_t side_key;
uint64_t castling_keys[16];
uint64_t ep_keys[120];

void init_zobrist() {
    std::mt19937_64 rng(123456789ULL); // Fixed seed for reproducibility

    for (int p = 0; p < 13; p++) {
        for (int sq = 0; sq < 120; sq++) {
            piece_keys[p][sq] = rng();
        }
    }
    
    side_key = rng();
    
    for (int i = 0; i < 16; i++) {
        castling_keys[i] = rng();
    }
    
    for (int sq = 0; sq < 120; sq++) {
        ep_keys[sq] = rng();
    }
}
