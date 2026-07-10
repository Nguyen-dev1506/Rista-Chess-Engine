#include "eval.h"
#include "bitboard.h"
#include "magic.h"
#include <algorithm>
#include <iostream>

namespace Eval {
    const int PieceValueMG[6] = { 82, 337, 365, 477, 1025, 20000 };
    const int PieceValueEG[6] = { 94, 281, 297, 512,  936, 20000 };

    const int mg_pawn[64] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        -35, -1, -20, -23, -15, 24, 38, -22,
        -26, -4, -4, -10, 3, 3, 33, -12,
        -27, -2, -5, 12, 17, 6, 10, -25,
        -14, 13, 6, 21, 23, 12, 17, -23,
        -6, 7, 26, 31, 65, 56, 25, -20,
        98, 134, 61, 95, 68, 126, 34, -11,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    const int eg_pawn[64] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        13, 8, 8, 10, 13, 0, 2, -7,
        4, 7, -6, 1, 0, -5, -1, -8,
        13, 9, -3, -7, -7, -8, 3, -1,
        32, 24, 13, 5, -2, 4, 17, 17,
        94, 100, 85, 67, 56, 53, 82, 84,
        178, 173, 158, 134, 147, 132, 165, 187,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    const int mg_knight[64] = {
        -105, -21, -58, -33, -17, -28, -19, -23,
        -41, -21, -10, -8, -9, -11, -43, -22,
        -23, -9, 12, 10, 19, 17, 28, -9,
        -13, 4, 16, 13, 28, 19, 21, -8,
        -9, 17, 19, 53, 37, 69, 18, 22,
        -47, 60, 37, 65, 84, 129, 73, 44,
        -73, -41, 72, 36, 23, 62, 7, -17,
        -167, -89, -34, -49, 61, -97, -15, -107
    };

    const int eg_knight[64] = {
        -29, -51, -23, -15, -22, -18, -50, -64,
        -42, -20, -10, -5, -2, -20, -23, -44,
        -23, -3, -1, 15, 10, -3, -20, -22,
        -18, -6, 16, 25, 16, 17, 4, -18,
        -17, 3, 22, 22, 22, 11, 8, -18,
        -24, -20, 10, 9, -1, -9, -19, -41,
        -25, -8, -25, -2, -9, -25, -24, -52,
        -58, -38, -13, -28, -31, -27, -63, -99
    };

    const int mg_bishop[64] = {
        -33, -3, -14, -21, -13, -12, -39, -21,
        4, 15, 16, 0, 7, 21, 33, 1,
        0, 15, 15, 15, 14, 27, 18, 10,
        -6, 13, 13, 26, 34, 12, 10, 4,
        -4, 5, 19, 50, 37, 37, 7, -2,
        -16, 37, 43, 40, 35, 50, 37, -2,
        -26, 16, -18, -13, 30, 59, 18, -47,
        -29, 4, -82, -37, -25, -42, 7, -8
    };

    const int eg_bishop[64] = {
        -23, -9, -23, -5, -9, -16, -5, -17,
        -14, -18, -7, -1, 4, -9, -15, -27,
        -12, -3, 8, 10, 13, 3, -7, -15,
        -6, 3, 13, 19, 7, 10, -3, -9,
        -3, 9, 12, 9, 14, 10, 3, 2,
        2, -8, 0, -1, -2, 6, 0, 4,
        -8, -4, 7, -12, -3, -13, -4, -14,
        -14, -21, -11, -8, -7, -9, -17, -24
    };

    const int mg_rook[64] = {
        -19, -13, 1, 17, 16, 7, -37, -26,
        -44, -16, -20, -9, -1, 11, -6, -71,
        -45, -25, -16, -17, 3, 0, -5, -33,
        -36, -26, -12, -1, 9, -7, 6, -23,
        -24, -11, 7, 26, 24, 35, -8, -20,
        -5, 19, 26, 36, 17, 45, 61, 16,
        27, 32, 58, 62, 80, 67, 26, 44,
        32, 42, 32, 51, 63, 9, 31, 43
    };

    const int eg_rook[64] = {
        -9, 2, 3, -1, -5, -13, 4, -20,
        -6, -6, 0, 2, -9, -9, -11, -3,
        -4, 0, -5, -1, -7, -12, -8, -16,
        3, 5, 8, 4, -5, -6, -8, -11,
        4, 3, 13, 1, 2, 1, -1, 2,
        7, 7, 7, 5, 4, -3, -5, -3,
        11, 13, 13, 11, -3, 3, 8, 3,
        13, 10, 18, 15, 12, 12, 8, 5
    };

    const int mg_queen[64] = {
        -1, -18, -9, 10, -15, -25, -31, -50,
        -35, -8, 11, 2, 8, 15, -3, 1,
        -14, 2, -11, -2, -5, 2, 14, 5,
        -9, -26, -9, -10, -2, -4, 3, -3,
        -27, -27, -16, -16, -1, 17, -2, 1,
        -13, -17, 7, 8, 29, 56, 47, 57,
        -24, -39, -5, 1, -16, 57, 28, 54,
        -28, 0, 29, 12, 59, 44, 43, 45
    };

    const int eg_queen[64] = {
        -33, -28, -22, -43, -5, -32, -20, -41,
        -22, -23, -30, -16, -16, -23, -36, -32,
        -16, -27, 15, 6, 9, 17, 10, 5,
        -18, 28, 19, 47, 31, 34, 12, 11,
        3, 22, 24, 45, 57, 40, 57, 36,
        -20, 6, 9, 49, 47, 35, 19, 9,
        -17, 20, 32, 41, 58, 25, 30, 0,
        -9, 22, 22, 27, 27, 19, 10, 20
    };

    const int mg_king[64] = {
        -15, 36, 12, -54, 8, -28, 24, 14,
        1, 7, -8, -64, -43, -16, 9, 8,
        -14, -14, -22, -46, -44, -30, -15, -27,
        -49, -1, -27, -39, -46, -44, -33, -51,
        -17, -20, -12, -27, -30, -25, -14, -36,
        -9, 24, 2, -16, -20, 6, 22, -22,
        29, -1, -20, -7, -8, -4, -38, -29,
        -65, 23, 16, -15, -56, -34, 2, 13
    };

    const int eg_king[64] = {
        -53, -34, -21, -11, -28, -14, -24, -43,
        -27, -11, 4, 13, 14, 4, -5, -17,
        -19, -3, 11, 21, 23, 16, 7, -9,
        -18, -4, 21, 24, 27, 23, 9, -11,
        -8, 22, 24, 27, 26, 33, 26, 3,
        10, 17, 23, 15, 20, 45, 44, 13,
        -12, 17, 14, 17, 17, 38, 23, 11,
        -74, -35, -18, -18, -11, 15, 4, -17
    };


    const int* mg_table[6] = {mg_pawn, mg_knight, mg_bishop, mg_rook, mg_queen, mg_king};
    const int* eg_table[6] = {eg_pawn, eg_knight, eg_bishop, eg_rook, eg_queen, eg_king};

    int get_sq(Color c, Square sq) {
        return (c == BLACK) ? (sq ^ 56) : sq;
    }

    inline int chebyshev(int sq1, int sq2) {
        int f1 = sq1 % 8, r1 = sq1 / 8;
        int f2 = sq2 % 8, r2 = sq2 / 8;
        return std::max(std::abs(f1 - f2), std::abs(r1 - r2));
    }

    U64 get_front_span(Color c, Square sq) {
        U64 file = FileA << (sq % 8);
        U64 adj = 0;
        if ((sq % 8) > 0) adj |= (file >> 1);
        if ((sq % 8) < 7) adj |= (file << 1);
        U64 mask = file | adj;
        if (c == WHITE) {
            int r = sq / 8;
            U64 rank_mask = ~((1ULL << ((r + 1) * 8)) - 1);
            return mask & rank_mask;
        } else {
            int r = sq / 8;
            U64 rank_mask = (1ULL << (r * 8)) - 1;
            return mask & rank_mask;
        }
    }

    int evaluate(const Board& board) {
        int mg[2] = {0, 0};
        int eg[2] = {0, 0};
        int gamePhase = 0;

        int num_pieces = popcount(board.pieces[W_KNIGHT]) + popcount(board.pieces[W_BISHOP]) + 
                         popcount(board.pieces[W_ROOK]) + popcount(board.pieces[W_QUEEN]) +
                         popcount(board.pieces[B_KNIGHT]) + popcount(board.pieces[B_BISHOP]) + 
                         popcount(board.pieces[B_ROOK]) + popcount(board.pieces[B_QUEEN]) +
                         popcount(board.pieces[W_PAWN]) + popcount(board.pieces[B_PAWN]);
        
        if (num_pieces == 0) return 0; // KvK
        if (num_pieces == 1 && (popcount(board.pieces[W_KNIGHT]) == 1 || popcount(board.pieces[B_KNIGHT]) == 1 || 
                                popcount(board.pieces[W_BISHOP]) == 1 || popcount(board.pieces[B_BISHOP]) == 1)) {
            return 0; // KvKN or KvKB
        }

        int w_king_sq = lsb(board.pieces[W_KING]);
        int b_king_sq = lsb(board.pieces[B_KING]);

        // Mobility, King Safety, Pawn Structure scores
        int pawn_mg[2] = {0, 0}, pawn_eg[2] = {0, 0};
        int mob_mg[2] = {0, 0}, mob_eg[2] = {0, 0};
        int ks_mg[2] = {0, 0}, ks_eg[2] = {0, 0};

        U64 pawns[2] = {board.pieces[W_PAWN], board.pieces[B_PAWN]};

        U64 enemy_pawn_attacks[2] = {0, 0};
        U64 wp = pawns[WHITE];
        while (wp) {
            enemy_pawn_attacks[WHITE] |= Bitboards::PawnAttacks[WHITE][pop_lsb(wp)];
        }
        U64 bp = pawns[BLACK];
        while (bp) {
            enemy_pawn_attacks[BLACK] |= Bitboards::PawnAttacks[BLACK][pop_lsb(bp)];
        }

        // Bishop pair bonus
        if (popcount(board.pieces[W_BISHOP]) >= 2) { mg[WHITE] += 40; eg[WHITE] += 50; }
        if (popcount(board.pieces[B_BISHOP]) >= 2) { mg[BLACK] += 40; eg[BLACK] += 50; }

        for (int c = WHITE; c <= BLACK; ++c) {
            Color color = static_cast<Color>(c);
            Color opp = (color == WHITE) ? BLACK : WHITE;
            
            U64 our_pawns = pawns[color];
            U64 opp_pawns = pawns[opp];

            for (int pt = PAWN; pt <= KING; ++pt) {
                Piece p = static_cast<Piece>((color == WHITE) ? pt : pt + 6);
                U64 bb = board.pieces[p];
                int count = popcount(bb);
                
                mg[color] += count * PieceValueMG[pt];
                eg[color] += count * PieceValueEG[pt];

                if (pt == KNIGHT || pt == BISHOP) gamePhase += 1 * count;
                else if (pt == ROOK) gamePhase += 2 * count;
                else if (pt == QUEEN) gamePhase += 4 * count;
                
                while (bb) {
                    int sq = pop_lsb(bb);
                    int map_sq = get_sq(color, static_cast<Square>(sq));
                    mg[color] += mg_table[pt][map_sq];
                    eg[color] += eg_table[pt][map_sq];

                    // --- Positional Evaluation ---
                    if (pt == PAWN) {
                        U64 file_mask = FileA << (sq % 8);
                        U64 adj_files = 0;
                        if ((sq % 8) > 0) adj_files |= (file_mask >> 1);
                        if ((sq % 8) < 7) adj_files |= (file_mask << 1);

                        // Isolated
                        if ((our_pawns & adj_files) == 0) {
                            pawn_mg[color] -= 15;
                            pawn_eg[color] -= 25;
                        }
                        // Doubled
                        if (our_pawns & file_mask & ~((1ULL << sq) | ((1ULL << sq) - 1))) {
                            pawn_mg[color] -= 10;
                            pawn_eg[color] -= 15;
                        }
                        // Passed
                        U64 front = get_front_span(color, static_cast<Square>(sq));
                        if ((opp_pawns & front) == 0) {
                            int r = (color == WHITE) ? (sq / 8) : 7 - (sq / 8);
                            
                            // Use reasonable arrays/multipliers instead of quadratic explosions
                            const int passed_bonus_mg[8] = {0, 5, 10, 20, 35, 60, 90, 0};
                            const int passed_bonus_eg[8] = {0, 10, 20, 40, 70, 120, 180, 0};
                            
                            int bonus_mg = passed_bonus_mg[r];
                            int bonus_eg = passed_bonus_eg[r];

                            // Check Blockade
                            Square forward_sq = static_cast<Square>((color == WHITE) ? sq + 8 : sq - 8);
                            if (board.piece_on(forward_sq) != EMPTY_PIECE) {
                                bonus_mg /= 2;
                                bonus_eg /= 2;
                            }

                            // 1. King-to-Passed-Pawn distance
                            int our_king = (color == WHITE) ? w_king_sq : b_king_sq;
                            int opp_king = (color == WHITE) ? b_king_sq : w_king_sq;
                            int our_dist = chebyshev(sq, our_king);
                            int opp_dist = chebyshev(sq, opp_king);

                            // Bonus for our king being close, penalty for enemy king being close
                            bonus_eg += (8 - our_dist) * 2;
                            bonus_eg -= (8 - opp_dist) * 5;

                            // 2. Rule of the Square
                            int dist_to_prom = 7 - r;
                            // If pawn is on start square, it can move 2
                            if (r == 1) dist_to_prom--;
                            
                            // Adjust for whose turn it is
                            int eff_dist_to_prom = dist_to_prom;
                            if (board.side_to_move != color) eff_dist_to_prom++;
                            
                            U64 opp_major_minor = board.occ(opp) & ~opp_pawns & ~board.pieces[(opp == WHITE) ? W_KING : B_KING];

                            if (opp_major_minor == 0 && opp_dist > eff_dist_to_prom) {
                                // Enemy king cannot catch the pawn, and has no pieces to stop it!
                                bonus_eg += 250; // Massive bonus
                            }

                            pawn_mg[color] += bonus_mg;
                            pawn_eg[color] += bonus_eg;
                        }
                    }

                    if (pt == ROOK) {
                        U64 file_mask = FileA << (sq % 8);
                        if ((our_pawns & file_mask) == 0) {
                            if ((opp_pawns & file_mask) == 0) {
                                // Open file
                                mob_mg[color] += 45;
                                mob_eg[color] += 25;
                            } else {
                                // Semi-open file
                                mob_mg[color] += 20;
                                mob_eg[color] += 10;
                            }
                        }
                    }

                    // Mobility & Outposts
                    if (pt == KNIGHT || pt == BISHOP || pt == ROOK || pt == QUEEN) {
                        U64 attacks = 0;
                        if (pt == KNIGHT) attacks = Bitboards::KnightAttacks[sq];
                        else if (pt == BISHOP) attacks = Magic::get_bishop_attacks(static_cast<Square>(sq), board.occ());
                        else if (pt == ROOK) attacks = Magic::get_rook_attacks(static_cast<Square>(sq), board.occ());
                        else if (pt == QUEEN) attacks = Magic::get_queen_attacks(static_cast<Square>(sq), board.occ());

                        U64 safe_moves = attacks & ~board.occ(color) & ~enemy_pawn_attacks[opp];
                        // Very simple mobility score
                        int mob = popcount(safe_moves);
                        mob_mg[color] += mob * 3;
                        mob_eg[color] += mob * 4;

                        // Outposts (Knights and Bishops on ranks 4,5,6 defended by pawn)
                        if (pt == KNIGHT || pt == BISHOP) {
                            int r = (color == WHITE) ? (sq / 8) : 7 - (sq / 8);
                            if (r >= 3 && r <= 5) { // rank 4, 5, 6
                                U64 pawn_defenders = (color == WHITE) ? 
                                    (((1ULL << sq) >> 9) & ~FileH) | (((1ULL << sq) >> 7) & ~FileA) :
                                    (((1ULL << sq) << 7) & ~FileH) | (((1ULL << sq) << 9) & ~FileA);
                                
                                if (our_pawns & pawn_defenders) {
                                    mob_mg[color] += 30;
                                    mob_eg[color] += 20;
                                }
                            }
                        }
                    }

                    // King Safety (Pawn Shield)
                    if (pt == KING) {
                        // Only care about king safety in middlegame
                        int r = (color == WHITE) ? (sq / 8) : 7 - (sq / 8);
                        if (r <= 2) { // King is on rank 1, 2, 3
                            U64 file_mask = FileA << (sq % 8);
                            U64 adj_files = 0;
                            if ((sq % 8) > 0) adj_files |= (file_mask >> 1);
                            if ((sq % 8) < 7) adj_files |= (file_mask << 1);
                            
                            U64 rank_mask = (color == WHITE) ? 0x00000000FFFFFF00ULL : 0x00FFFFFF00000000ULL;
                            U64 ks_zone = (file_mask | adj_files) & rank_mask;
                            const int shield_penalty[4] = { -80, -45, -15, 0 };
                            int shield_count = std::min(3, popcount(our_pawns & ks_zone));
                            ks_mg[color] += shield_penalty[shield_count];
                            
                            // Open files near king (Dangerous if WE don't have a pawn)
                            int open_files = 0;
                            if ((our_pawns & file_mask) == 0) open_files++;
                            if ((sq % 8) > 0 && (our_pawns & (file_mask >> 1)) == 0) open_files++;
                            if ((sq % 8) < 7 && (our_pawns & (file_mask << 1)) == 0) open_files++;
                            ks_mg[color] -= open_files * 40; // Extremely strong penalty for missing our pawns
                            
                            // Heavy penalty if enemy rook/queen is on these open files
                            U64 enemy_heavy = board.pieces[(color == WHITE) ? B_ROOK : W_ROOK] | board.pieces[(color == WHITE) ? B_QUEEN : W_QUEEN];
                            if (enemy_heavy & file_mask) ks_mg[color] -= 30;
                            if ((sq % 8) > 0 && (enemy_heavy & (file_mask >> 1))) ks_mg[color] -= 20;
                            if ((sq % 8) < 7 && (enemy_heavy & (file_mask << 1))) ks_mg[color] -= 20;

                            // King Danger from enemy pieces attacking king ring
                            U64 king_ring = Bitboards::KingAttacks[sq];
                            int attack_units = 0;
                            
                            U64 opp_knights = board.pieces[(color == WHITE) ? B_KNIGHT : W_KNIGHT];
                            while (opp_knights) {
                                int n_sq = pop_lsb(opp_knights);
                                if (Bitboards::KnightAttacks[n_sq] & king_ring) attack_units += 2;
                            }
                            U64 opp_bishops = board.pieces[(color == WHITE) ? B_BISHOP : W_BISHOP];
                            while (opp_bishops) {
                                int b_sq = pop_lsb(opp_bishops);
                                if (Magic::get_bishop_attacks(static_cast<Square>(b_sq), board.occ()) & king_ring) attack_units += 2;
                            }
                            U64 opp_rooks = board.pieces[(color == WHITE) ? B_ROOK : W_ROOK];
                            while (opp_rooks) {
                                int r_sq = pop_lsb(opp_rooks);
                                if (Magic::get_rook_attacks(static_cast<Square>(r_sq), board.occ()) & king_ring) attack_units += 3;
                            }
                            U64 opp_queens = board.pieces[(color == WHITE) ? B_QUEEN : W_QUEEN];
                            while (opp_queens) {
                                int q_sq = pop_lsb(opp_queens);
                                if (Magic::get_queen_attacks(static_cast<Square>(q_sq), board.occ()) & king_ring) attack_units += 5;
                            }
                            
                            if (attack_units > 0) {
                                const int danger_table[15] = {0, 0, 10, 20, 35, 55, 80, 110, 145, 185, 230, 280, 335, 400, 470};
                                int danger = danger_table[std::min(14, attack_units)];
                                ks_mg[color] -= danger;
                            }
                        }
                    }
                }
            }
        }

        mg[WHITE] += pawn_mg[WHITE] + mob_mg[WHITE] + ks_mg[WHITE];
        eg[WHITE] += pawn_eg[WHITE] + mob_eg[WHITE] + ks_eg[WHITE];
        mg[BLACK] += pawn_mg[BLACK] + mob_mg[BLACK] + ks_mg[BLACK];
        eg[BLACK] += pawn_eg[BLACK] + mob_eg[BLACK] + ks_eg[BLACK];

        if (gamePhase >= 20) {
            int w_undeveloped = 0;
            if (board.piece_on(B1) == W_KNIGHT) { mg[WHITE] -= 15; w_undeveloped++; }
            if (board.piece_on(G1) == W_KNIGHT) { mg[WHITE] -= 15; w_undeveloped++; }
            if (board.piece_on(C1) == W_BISHOP) { mg[WHITE] -= 15; w_undeveloped++; }
            if (board.piece_on(F1) == W_BISHOP) { mg[WHITE] -= 15; w_undeveloped++; }
            if (board.piece_on(D1) != W_QUEEN && w_undeveloped >= 2) { mg[WHITE] -= 20; }

            int b_undeveloped = 0;
            if (board.piece_on(B8) == B_KNIGHT) { mg[BLACK] -= 15; b_undeveloped++; }
            if (board.piece_on(G8) == B_KNIGHT) { mg[BLACK] -= 15; b_undeveloped++; }
            if (board.piece_on(C8) == B_BISHOP) { mg[BLACK] -= 15; b_undeveloped++; }
            if (board.piece_on(F8) == B_BISHOP) { mg[BLACK] -= 15; b_undeveloped++; }
            if (board.piece_on(D8) != B_QUEEN && b_undeveloped >= 2) { mg[BLACK] -= 20; }
        }

        int mgScore = mg[WHITE] - mg[BLACK];
        int egScore = eg[WHITE] - eg[BLACK];
        
        // 3. Endgame King Mating Net (Push to edge)
        if (gamePhase <= 2) {
            if (egScore > 400) { // White is winning
                int opp_king_r = b_king_sq / 8;
                int opp_king_f = b_king_sq % 8;
                int edge_dist = std::min(opp_king_r, 7 - opp_king_r) + std::min(opp_king_f, 7 - opp_king_f);
                egScore += (7 - edge_dist) * 10;
                egScore += (7 - chebyshev(w_king_sq, b_king_sq)) * 5; // Bring our king closer
            } else if (egScore < -400) { // Black is winning
                int opp_king_r = w_king_sq / 8;
                int opp_king_f = w_king_sq % 8;
                int edge_dist = std::min(opp_king_r, 7 - opp_king_r) + std::min(opp_king_f, 7 - opp_king_f);
                egScore -= (7 - edge_dist) * 10;
                egScore -= (7 - chebyshev(b_king_sq, w_king_sq)) * 5;
            }
        }

        // 4. Endgame Scaling (Drawish positions)
        if (egScore > 0 && popcount(board.pieces[W_PAWN]) == 0) {
            if (popcount(board.pieces[W_ROOK]) == 0 && popcount(board.pieces[W_QUEEN]) == 0) {
                // White has only minor pieces and no pawns -> hard to win
                egScore /= 4;
                mgScore /= 4;
            }
        } else if (egScore < 0 && popcount(board.pieces[B_PAWN]) == 0) {
            if (popcount(board.pieces[B_ROOK]) == 0 && popcount(board.pieces[B_QUEEN]) == 0) {
                egScore /= 4;
                mgScore /= 4;
            }
        }
        
        int mgPhase = gamePhase;
        if (mgPhase > 24) mgPhase = 24;
        int egPhase = 24 - mgPhase;
        
        int score = (mgScore * mgPhase + egScore * egPhase) / 24;
        return (board.side_to_move == WHITE) ? score : -score;
    }
}
