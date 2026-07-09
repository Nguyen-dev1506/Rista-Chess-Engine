#ifndef MAGIC_H
#define MAGIC_H

#include "bitboard.h"

namespace Magic {
    extern U64 BishopMasks[64];
    extern U64 RookMasks[64];
    extern U64 BishopAttacks[64][512];
    extern U64 RookAttacks[64][4096];
    extern U64 BishopMagics[64];
    extern U64 RookMagics[64];
    extern int BishopShifts[64];
    extern int RookShifts[64];

    void init();

    inline U64 get_bishop_attacks(Square sq, U64 occupancy) {
        occupancy &= BishopMasks[sq];
        occupancy *= BishopMagics[sq];
        occupancy >>= BishopShifts[sq];
        return BishopAttacks[sq][occupancy];
    }

    inline U64 get_rook_attacks(Square sq, U64 occupancy) {
        occupancy &= RookMasks[sq];
        occupancy *= RookMagics[sq];
        occupancy >>= RookShifts[sq];
        return RookAttacks[sq][occupancy];
    }

    inline U64 get_queen_attacks(Square sq, U64 occupancy) {
        return get_bishop_attacks(sq, occupancy) | get_rook_attacks(sq, occupancy);
    }
}

#endif
