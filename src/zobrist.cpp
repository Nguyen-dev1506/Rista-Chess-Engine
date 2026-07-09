#include "zobrist.h"
#include <random>

namespace Zobrist {
    U64 piece_keys[12][64];
    U64 enpassant_keys[64];
    U64 castle_keys[16];
    U64 side_key;

    U64 random_u64_z() {
        static std::mt19937_64 rng(5489); // Default seed
        return rng();
    }

    void init() {
        for (int p = 0; p < 12; p++) {
            for (int sq = 0; sq < 64; sq++) {
                piece_keys[p][sq] = random_u64_z();
            }
        }
        for (int sq = 0; sq < 64; sq++) {
            enpassant_keys[sq] = random_u64_z();
        }
        for (int i = 0; i < 16; i++) {
            castle_keys[i] = random_u64_z();
        }
        side_key = random_u64_z();
    }
}
