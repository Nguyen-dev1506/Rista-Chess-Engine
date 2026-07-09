#ifndef BITBOARD_H
#define BITBOARD_H

#include "types.h"
#include <bit>

inline void set_bit(U64& bb, Square sq) { bb |= (1ULL << sq); }
inline void clear_bit(U64& bb, Square sq) { bb &= ~(1ULL << sq); }
inline bool get_bit(U64 bb, Square sq) { return (bb & (1ULL << sq)) != 0; }
inline int popcount(U64 bb) { return std::popcount(bb); }
inline int lsb(U64 bb) { return std::countr_zero(bb); }
inline int pop_lsb(U64& bb) {
    int sq = lsb(bb);
    bb &= bb - 1;
    return sq;
}

constexpr U64 FileA = 0x0101010101010101ULL;
constexpr U64 FileB = FileA << 1;
constexpr U64 FileC = FileA << 2;
constexpr U64 FileD = FileA << 3;
constexpr U64 FileE = FileA << 4;
constexpr U64 FileF = FileA << 5;
constexpr U64 FileG = FileA << 6;
constexpr U64 FileH = FileA << 7;

constexpr U64 Rank1 = 0x00000000000000FFULL;
constexpr U64 Rank2 = Rank1 << 8;
constexpr U64 Rank3 = Rank1 << 16;
constexpr U64 Rank4 = Rank1 << 24;
constexpr U64 Rank5 = Rank1 << 32;
constexpr U64 Rank6 = Rank1 << 40;
constexpr U64 Rank7 = Rank1 << 48;
constexpr U64 Rank8 = Rank1 << 56;

namespace Bitboards {
    extern U64 KnightAttacks[64];
    extern U64 KingAttacks[64];
    extern U64 PawnAttacks[2][64];
    
    void init();
    
    inline U64 get_knight_attacks(Square sq) { return KnightAttacks[sq]; }
    inline U64 get_king_attacks(Square sq) { return KingAttacks[sq]; }
    inline U64 get_pawn_attacks(Color c, Square sq) { return PawnAttacks[c][sq]; }
}

#endif
