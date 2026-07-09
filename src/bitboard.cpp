#include "bitboard.h"

namespace Bitboards {
    U64 KnightAttacks[64];
    U64 KingAttacks[64];
    U64 PawnAttacks[2][64];

    U64 mask_knight(Square sq) {
        U64 bb = 0ULL, attacks = 0ULL;
        set_bit(bb, sq);
        attacks |= (bb >> 17) & 0x7F7F7F7F7F7F7F7FULL;
        attacks |= (bb >> 15) & 0xFEFEFEFEFEFEFEFEULL;
        attacks |= (bb >> 10) & 0x3F3F3F3F3F3F3F3FULL;
        attacks |= (bb >>  6) & 0xFCFCFCFCFCFCFCFCULL;
        attacks |= (bb << 17) & 0xFEFEFEFEFEFEFEFEULL;
        attacks |= (bb << 15) & 0x7F7F7F7F7F7F7F7FULL;
        attacks |= (bb << 10) & 0xFCFCFCFCFCFCFCFCULL;
        attacks |= (bb <<  6) & 0x3F3F3F3F3F3F3F3FULL;
        return attacks;
    }

    U64 mask_king(Square sq) {
        U64 bb = 0ULL, attacks = 0ULL;
        set_bit(bb, sq);
        attacks |= (bb >> 9) & 0x7F7F7F7F7F7F7F7FULL;
        attacks |= (bb >> 8);
        attacks |= (bb >> 7) & 0xFEFEFEFEFEFEFEFEULL;
        attacks |= (bb >> 1) & 0x7F7F7F7F7F7F7F7FULL;
        attacks |= (bb << 9) & 0xFEFEFEFEFEFEFEFEULL;
        attacks |= (bb << 8);
        attacks |= (bb << 7) & 0x7F7F7F7F7F7F7F7FULL;
        attacks |= (bb << 1) & 0xFEFEFEFEFEFEFEFEULL;
        return attacks;
    }

    U64 mask_pawn(Color c, Square sq) {
        U64 bb = 0ULL, attacks = 0ULL;
        set_bit(bb, sq);
        if (c == WHITE) {
            attacks |= (bb << 7) & 0x7F7F7F7F7F7F7F7FULL;
            attacks |= (bb << 9) & 0xFEFEFEFEFEFEFEFEULL;
        } else {
            attacks |= (bb >> 7) & 0xFEFEFEFEFEFEFEFEULL;
            attacks |= (bb >> 9) & 0x7F7F7F7F7F7F7F7FULL;
        }
        return attacks;
    }

    void init() {
        for (int sq = 0; sq < 64; sq++) {
            KnightAttacks[sq] = mask_knight((Square)sq);
            KingAttacks[sq] = mask_king((Square)sq);
            PawnAttacks[WHITE][sq] = mask_pawn(WHITE, (Square)sq);
            PawnAttacks[BLACK][sq] = mask_pawn(BLACK, (Square)sq);
        }
    }
}
