#include "movegen.h"
#include "magic.h"

namespace MoveGen {
    void generate_pawn_moves(const Board& board, MoveList& list, Color us, bool captures_only) {
        U64 pawns = board.pieces[us == WHITE ? W_PAWN : B_PAWN];
        U64 empty = ~board.occ();
        U64 enemies = board.occ(us == WHITE ? BLACK : WHITE);
        
        // Single push
        U64 push1 = (us == WHITE) ? ((pawns << 8) & empty) : ((pawns >> 8) & empty);
        U64 push2 = 0;
        
        if (!captures_only) {
            // Double push
            U64 rank3 = (us == WHITE) ? 0x0000000000FF0000ULL : 0;
            U64 rank6 = (us == BLACK) ? 0x0000FF0000000000ULL : 0;
            push2 = (us == WHITE) ? (((push1 & rank3) << 8) & empty) : (((push1 & rank6) >> 8) & empty);
            
            U64 p1 = push1, p2 = push2;
            while (p1) {
                int to = pop_lsb(p1);
                int from = (us == WHITE) ? to - 8 : to + 8;
                if ((us == WHITE && to >= A8) || (us == BLACK && to <= H1)) {
                    list.add(Move(static_cast<Square>(from), static_cast<Square>(to), Q_PROMO));
                    list.add(Move(static_cast<Square>(from), static_cast<Square>(to), R_PROMO));
                    list.add(Move(static_cast<Square>(from), static_cast<Square>(to), B_PROMO));
                    list.add(Move(static_cast<Square>(from), static_cast<Square>(to), N_PROMO));
                } else {
                    list.add(Move(static_cast<Square>(from), static_cast<Square>(to), QUIET));
                }
            }
            while (p2) {
                int to = pop_lsb(p2);
                int from = (us == WHITE) ? to - 16 : to + 16;
                list.add(Move(static_cast<Square>(from), static_cast<Square>(to), DOUBLE_PAWN));
            }
        }
        
        // Captures
        U64 cap_left = (us == WHITE) ? ((pawns << 7) & 0x7F7F7F7F7F7F7F7FULL) : ((pawns >> 9) & 0x7F7F7F7F7F7F7F7FULL);
        U64 cap_right = (us == WHITE) ? ((pawns << 9) & 0xFEFEFEFEFEFEFEFEULL) : ((pawns >> 7) & 0xFEFEFEFEFEFEFEFEULL);
        
        U64 cap_l = cap_left & enemies;
        U64 cap_r = cap_right & enemies;
        
        while (cap_l) {
            int to = pop_lsb(cap_l);
            int from = (us == WHITE) ? to - 7 : to + 9;
            if ((us == WHITE && to >= A8) || (us == BLACK && to <= H1)) {
                list.add(Move(static_cast<Square>(from), static_cast<Square>(to), Q_PROMO_CAP));
                list.add(Move(static_cast<Square>(from), static_cast<Square>(to), R_PROMO_CAP));
                list.add(Move(static_cast<Square>(from), static_cast<Square>(to), B_PROMO_CAP));
                list.add(Move(static_cast<Square>(from), static_cast<Square>(to), N_PROMO_CAP));
            } else {
                list.add(Move(static_cast<Square>(from), static_cast<Square>(to), CAPTURE));
            }
        }
        
        while (cap_r) {
            int to = pop_lsb(cap_r);
            int from = (us == WHITE) ? to - 9 : to + 7;
            if ((us == WHITE && to >= A8) || (us == BLACK && to <= H1)) {
                list.add(Move(static_cast<Square>(from), static_cast<Square>(to), Q_PROMO_CAP));
                list.add(Move(static_cast<Square>(from), static_cast<Square>(to), R_PROMO_CAP));
                list.add(Move(static_cast<Square>(from), static_cast<Square>(to), B_PROMO_CAP));
                list.add(Move(static_cast<Square>(from), static_cast<Square>(to), N_PROMO_CAP));
            } else {
                list.add(Move(static_cast<Square>(from), static_cast<Square>(to), CAPTURE));
            }
        }
        
        // En Passant
        if (board.en_passant != NO_SQ) {
            U64 ep = (1ULL << board.en_passant);
            U64 ep_l = cap_left & ep;
            U64 ep_r = cap_right & ep;
            while (ep_l) {
                int to = pop_lsb(ep_l);
                int from = (us == WHITE) ? to - 7 : to + 9;
                list.add(Move(static_cast<Square>(from), static_cast<Square>(to), EP_CAPTURE));
            }
            while (ep_r) {
                int to = pop_lsb(ep_r);
                int from = (us == WHITE) ? to - 9 : to + 7;
                list.add(Move(static_cast<Square>(from), static_cast<Square>(to), EP_CAPTURE));
            }
        }
    }

    void generate_piece_moves(const Board& board, MoveList& list, Piece p, bool captures_only) {
        Color us = piece_color(p);
        U64 pieces = board.pieces[p];
        U64 enemies = board.occ(us == WHITE ? BLACK : WHITE);
        U64 empty = ~board.occ();
        U64 mask = captures_only ? enemies : (enemies | empty);
        
        while (pieces) {
            int from = pop_lsb(pieces);
            U64 attacks = 0;
            switch (piece_type(p)) {
                case KNIGHT: attacks = Bitboards::KnightAttacks[from]; break;
                case BISHOP: attacks = Magic::get_bishop_attacks(static_cast<Square>(from), board.occ()); break;
                case ROOK: attacks = Magic::get_rook_attacks(static_cast<Square>(from), board.occ()); break;
                case QUEEN: attacks = Magic::get_queen_attacks(static_cast<Square>(from), board.occ()); break;
                case KING: attacks = Bitboards::KingAttacks[from]; break;
                default: break;
            }
            
            attacks &= mask;
            while (attacks) {
                int to = pop_lsb(attacks);
                uint16_t flag = (enemies & (1ULL << to)) ? CAPTURE : QUIET;
                list.add(Move(static_cast<Square>(from), static_cast<Square>(to), flag));
            }
        }
    }

    void generate_pseudo_legal(const Board& board, MoveList& list, bool captures_only) {
        Color us = board.side_to_move;
        
        generate_pawn_moves(board, list, us, captures_only);
        
        generate_piece_moves(board, list, us == WHITE ? W_KNIGHT : B_KNIGHT, captures_only);
        generate_piece_moves(board, list, us == WHITE ? W_BISHOP : B_BISHOP, captures_only);
        generate_piece_moves(board, list, us == WHITE ? W_ROOK : B_ROOK, captures_only);
        generate_piece_moves(board, list, us == WHITE ? W_QUEEN : B_QUEEN, captures_only);
        generate_piece_moves(board, list, us == WHITE ? W_KING : B_KING, captures_only);
        
        if (!captures_only) {
            // Castling
            if (us == WHITE && !board.is_in_check(WHITE)) {
                if (board.castle_rights & WK) {
                    if (!get_bit(board.occ(), F1) && !get_bit(board.occ(), G1)) {
                        if (!board.is_attacked(F1, BLACK) && !board.is_attacked(G1, BLACK)) {
                            list.add(Move(E1, G1, KING_CASTLE));
                        }
                    }
                }
                if (board.castle_rights & WQ) {
                    if (!get_bit(board.occ(), D1) && !get_bit(board.occ(), C1) && !get_bit(board.occ(), B1)) {
                        if (!board.is_attacked(D1, BLACK) && !board.is_attacked(C1, BLACK)) {
                            list.add(Move(E1, C1, QUEEN_CASTLE));
                        }
                    }
                }
            } else if (us == BLACK && !board.is_in_check(BLACK)) {
                if (board.castle_rights & BK) {
                    if (!get_bit(board.occ(), F8) && !get_bit(board.occ(), G8)) {
                        if (!board.is_attacked(F8, WHITE) && !board.is_attacked(G8, WHITE)) {
                            list.add(Move(E8, G8, KING_CASTLE));
                        }
                    }
                }
                if (board.castle_rights & BQ) {
                    if (!get_bit(board.occ(), D8) && !get_bit(board.occ(), C8) && !get_bit(board.occ(), B8)) {
                        if (!board.is_attacked(D8, WHITE) && !board.is_attacked(C8, WHITE)) {
                            list.add(Move(E8, C8, QUEEN_CASTLE));
                        }
                    }
                }
            }
        }
    }
}
