#include "magic.h"
#include <random>
#include <cstring>

namespace Magic {
    U64 BishopMasks[64];
    U64 RookMasks[64];
    U64 BishopAttacks[64][512];
    U64 RookAttacks[64][4096];
    U64 BishopMagics[64];
    U64 RookMagics[64];
    int BishopShifts[64];
    int RookShifts[64];

    U64 random_u64() {
        static std::mt19937_64 rng(12345); // Fixed seed for reproducibility
        return rng() & rng() & rng(); // Sparse random numbers
    }

    U64 rook_mask(int sq) {
        U64 r = 0;
        int tr = sq / 8, tc = sq % 8;
        for (int r1 = tr + 1; r1 <= 6; r1++) set_bit(r, (Square)(r1 * 8 + tc));
        for (int r1 = tr - 1; r1 >= 1; r1--) set_bit(r, (Square)(r1 * 8 + tc));
        for (int c1 = tc + 1; c1 <= 6; c1++) set_bit(r, (Square)(tr * 8 + c1));
        for (int c1 = tc - 1; c1 >= 1; c1--) set_bit(r, (Square)(tr * 8 + c1));
        return r;
    }

    U64 bishop_mask(int sq) {
        U64 r = 0;
        int tr = sq / 8, tc = sq % 8;
        for (int r1 = tr + 1, c1 = tc + 1; r1 <= 6 && c1 <= 6; r1++, c1++) set_bit(r, (Square)(r1 * 8 + c1));
        for (int r1 = tr + 1, c1 = tc - 1; r1 <= 6 && c1 >= 1; r1++, c1--) set_bit(r, (Square)(r1 * 8 + c1));
        for (int r1 = tr - 1, c1 = tc + 1; r1 >= 1 && c1 <= 6; r1--, c1++) set_bit(r, (Square)(r1 * 8 + c1));
        for (int r1 = tr - 1, c1 = tc - 1; r1 >= 1 && c1 >= 1; r1--, c1--) set_bit(r, (Square)(r1 * 8 + c1));
        return r;
    }

    U64 rook_attack(int sq, U64 block) {
        U64 r = 0;
        int tr = sq / 8, tc = sq % 8;
        for (int r1 = tr + 1; r1 <= 7; r1++) { set_bit(r, (Square)(r1 * 8 + tc)); if (block & (1ULL << (r1 * 8 + tc))) break; }
        for (int r1 = tr - 1; r1 >= 0; r1--) { set_bit(r, (Square)(r1 * 8 + tc)); if (block & (1ULL << (r1 * 8 + tc))) break; }
        for (int c1 = tc + 1; c1 <= 7; c1++) { set_bit(r, (Square)(tr * 8 + c1)); if (block & (1ULL << (tr * 8 + c1))) break; }
        for (int c1 = tc - 1; c1 >= 0; c1--) { set_bit(r, (Square)(tr * 8 + c1)); if (block & (1ULL << (tr * 8 + c1))) break; }
        return r;
    }

    U64 bishop_attack(int sq, U64 block) {
        U64 r = 0;
        int tr = sq / 8, tc = sq % 8;
        for (int r1 = tr + 1, c1 = tc + 1; r1 <= 7 && c1 <= 7; r1++, c1++) { set_bit(r, (Square)(r1 * 8 + c1)); if (block & (1ULL << (r1 * 8 + c1))) break; }
        for (int r1 = tr + 1, c1 = tc - 1; r1 <= 7 && c1 >= 0; r1++, c1--) { set_bit(r, (Square)(r1 * 8 + c1)); if (block & (1ULL << (r1 * 8 + c1))) break; }
        for (int r1 = tr - 1, c1 = tc + 1; r1 >= 0 && c1 <= 7; r1--, c1++) { set_bit(r, (Square)(r1 * 8 + c1)); if (block & (1ULL << (r1 * 8 + c1))) break; }
        for (int r1 = tr - 1, c1 = tc - 1; r1 >= 0 && c1 >= 0; r1--, c1--) { set_bit(r, (Square)(r1 * 8 + c1)); if (block & (1ULL << (r1 * 8 + c1))) break; }
        return r;
    }

    U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask) {
        U64 occupancy = 0ULL;
        for (int count = 0; count < bits_in_mask; count++) {
            int sq = lsb(attack_mask);
            clear_bit(attack_mask, (Square)sq);
            if (index & (1 << count)) {
                occupancy |= (1ULL << sq);
            }
        }
        return occupancy;
    }

    U64 find_magic(int sq, int m, int is_bishop) {
        U64 mask = is_bishop ? bishop_mask(sq) : rook_mask(sq);
        int n = popcount(mask);
        U64 occupancies[4096];
        U64 attacks[4096];
        U64 used[4096];
        for (int i = 0; i < (1 << n); i++) {
            occupancies[i] = set_occupancy(i, n, mask);
            attacks[i] = is_bishop ? bishop_attack(sq, occupancies[i]) : rook_attack(sq, occupancies[i]);
        }
        for (int k = 0; k < 100000000; k++) {
            U64 magic = random_u64();
            if (popcount((mask * magic) & 0xFF00000000000000ULL) < 6) continue;
            std::memset(used, 0, sizeof(used));
            int i, fail = 0;
            for (i = 0; i < (1 << n); i++) {
                int magic_index = (int)((occupancies[i] * magic) >> (64 - m));
                if (used[magic_index] == 0) used[magic_index] = attacks[i];
                else if (used[magic_index] != attacks[i]) { fail = 1; break; }
            }
            if (!fail) return magic;
        }
        return 0ULL;
    }

    void init() {
        for (int sq = 0; sq < 64; sq++) {
            BishopMasks[sq] = bishop_mask(sq);
            RookMasks[sq] = rook_mask(sq);
            int b_bits = popcount(BishopMasks[sq]);
            int r_bits = popcount(RookMasks[sq]);
            BishopShifts[sq] = 64 - b_bits;
            RookShifts[sq] = 64 - r_bits;

            BishopMagics[sq] = find_magic(sq, b_bits, 1);
            RookMagics[sq] = find_magic(sq, r_bits, 0);

            for (int i = 0; i < (1 << b_bits); i++) {
                U64 occ = set_occupancy(i, b_bits, BishopMasks[sq]);
                int magic_index = (occ * BishopMagics[sq]) >> BishopShifts[sq];
                BishopAttacks[sq][magic_index] = bishop_attack(sq, occ);
            }
            for (int i = 0; i < (1 << r_bits); i++) {
                U64 occ = set_occupancy(i, r_bits, RookMasks[sq]);
                int magic_index = (occ * RookMagics[sq]) >> RookShifts[sq];
                RookAttacks[sq][magic_index] = rook_attack(sq, occ);
            }
        }
    }
}
