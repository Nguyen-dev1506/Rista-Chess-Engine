#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <cstdint>

extern uint64_t piece_keys[13][120];
extern uint64_t side_key;
extern uint64_t castling_keys[16];
extern uint64_t ep_keys[120];

void init_zobrist();

// Map a piece (-6 to 6) to index (0 to 12)
inline int get_piece_index(int piece) {
    return piece + 6;
}

#endif
