#include "movegen.h"
#include <cmath>

extern const int KNIGHT_OFFSETS[8];
extern const int BISHOP_OFFSETS[4];
extern const int ROOK_OFFSETS[4];
extern const int KING_OFFSETS[8];

void generate_pawn_moves(Board& board, std::vector<Move>& moves, int sq, int side) {
    bool is_white_turn = (side == WHITE);
    int dir = is_white_turn ? 10 : -10;
    int start_rank = is_white_turn ? 3 : 8; 
    int promo_rank = is_white_turn ? 9 : 2;
    
    int to = sq + dir;
    if (board.pieces[to] == EMPTY) {
        if (to / 10 == promo_rank) {
            moves.push_back(encode_move(sq, to, FLAG_PROMOTION));
        } else {
            moves.push_back(encode_move(sq, to, FLAG_NONE));
            if (sq / 10 == start_rank) {
                int to2 = to + dir;
                if (board.pieces[to2] == EMPTY) {
                    moves.push_back(encode_move(sq, to2, FLAG_NONE));
                }
            }
        }
    }
    
    int cap1 = sq + dir - 1;
    int cap2 = sq + dir + 1;
    
    if (board.pieces[cap1] != OFFBOARD) {
        if (is_white_turn && board.pieces[cap1] < 0) {
            if (cap1 / 10 == promo_rank) moves.push_back(encode_move(sq, cap1, FLAG_PROMOTION));
            else moves.push_back(encode_move(sq, cap1, FLAG_CAPTURE));
        } else if (!is_white_turn && board.pieces[cap1] > 0) {
            if (cap1 / 10 == promo_rank) moves.push_back(encode_move(sq, cap1, FLAG_PROMOTION));
            else moves.push_back(encode_move(sq, cap1, FLAG_CAPTURE));
        } else if (cap1 == board.en_passant) {
            moves.push_back(encode_move(sq, cap1, FLAG_NONE));
        }
    }
    
    if (board.pieces[cap2] != OFFBOARD) {
        if (is_white_turn && board.pieces[cap2] < 0) {
            if (cap2 / 10 == promo_rank) moves.push_back(encode_move(sq, cap2, FLAG_PROMOTION));
            else moves.push_back(encode_move(sq, cap2, FLAG_CAPTURE));
        } else if (!is_white_turn && board.pieces[cap2] > 0) {
            if (cap2 / 10 == promo_rank) moves.push_back(encode_move(sq, cap2, FLAG_PROMOTION));
            else moves.push_back(encode_move(sq, cap2, FLAG_CAPTURE));
        } else if (cap2 == board.en_passant) {
            moves.push_back(encode_move(sq, cap2, FLAG_NONE));
        }
    }
}

void generate_knight_moves(Board& board, std::vector<Move>& moves, int sq, int side) {
    bool is_white_turn = (side == WHITE);
    for (int offset : KNIGHT_OFFSETS) {
        int to_sq = sq + offset;
        int p = board.pieces[to_sq];
        if (p == OFFBOARD) continue;
        
        if (p == EMPTY) {
            moves.push_back(encode_move(sq, to_sq, FLAG_NONE));
        } else {
            if (is_white_turn && p < 0) {
                moves.push_back(encode_move(sq, to_sq, FLAG_CAPTURE));
            } else if (!is_white_turn && p > 0) {
                moves.push_back(encode_move(sq, to_sq, FLAG_CAPTURE));
            }
        }
    }
}

void generate_king_moves(Board& board, std::vector<Move>& moves, int sq, int side) {
    bool is_white_turn = (side == WHITE);
    for (int offset : KING_OFFSETS) {
        int to_sq = sq + offset;
        int p = board.pieces[to_sq];
        if (p == OFFBOARD) continue;
        
        if (p == EMPTY) {
            moves.push_back(encode_move(sq, to_sq, FLAG_NONE));
        } else {
            if (is_white_turn && p < 0) {
                moves.push_back(encode_move(sq, to_sq, FLAG_CAPTURE));
            } else if (!is_white_turn && p > 0) {
                moves.push_back(encode_move(sq, to_sq, FLAG_CAPTURE));
            }
        }
    }
    
    if (side == WHITE) {
        if ((board.castling_rights & WK_CASTLING) && board.pieces[F1] == EMPTY && board.pieces[G1] == EMPTY) {
            if (!board.is_square_attacked(E1, BLACK) && !board.is_square_attacked(F1, BLACK) && !board.is_square_attacked(G1, BLACK)) {
                moves.push_back(encode_move(E1, G1, FLAG_CASTLING));
            }
        }
        if ((board.castling_rights & WQ_CASTLING) && board.pieces[D1] == EMPTY && board.pieces[C1] == EMPTY && board.pieces[B1] == EMPTY) {
            if (!board.is_square_attacked(E1, BLACK) && !board.is_square_attacked(D1, BLACK) && !board.is_square_attacked(C1, BLACK)) {
                moves.push_back(encode_move(E1, C1, FLAG_CASTLING));
            }
        }
    } else {
        if ((board.castling_rights & BK_CASTLING) && board.pieces[F8] == EMPTY && board.pieces[G8] == EMPTY) {
            if (!board.is_square_attacked(E8, WHITE) && !board.is_square_attacked(F8, WHITE) && !board.is_square_attacked(G8, WHITE)) {
                moves.push_back(encode_move(E8, G8, FLAG_CASTLING));
            }
        }
        if ((board.castling_rights & BQ_CASTLING) && board.pieces[D8] == EMPTY && board.pieces[C8] == EMPTY && board.pieces[B8] == EMPTY) {
            if (!board.is_square_attacked(E8, WHITE) && !board.is_square_attacked(D8, WHITE) && !board.is_square_attacked(C8, WHITE)) {
                moves.push_back(encode_move(E8, C8, FLAG_CASTLING));
            }
        }
    }
}

void generate_sliding_moves(Board& board, std::vector<Move>& moves, int sq, int side, const int* offsets, int num_offsets) {
    bool is_white_turn = (side == WHITE);
    for (int i = 0; i < num_offsets; i++) {
        int dir = offsets[i];
        int to_sq = sq + dir;
        while (board.pieces[to_sq] != OFFBOARD) {
            if (board.pieces[to_sq] == EMPTY) {
                moves.push_back(encode_move(sq, to_sq, FLAG_NONE));
                to_sq += dir;
            } else {
                if (is_white_turn && board.pieces[to_sq] < 0) {
                    moves.push_back(encode_move(sq, to_sq, FLAG_CAPTURE));
                } else if (!is_white_turn && board.pieces[to_sq] > 0) {
                    moves.push_back(encode_move(sq, to_sq, FLAG_CAPTURE));
                }
                break;
            }
        }
    }
}

void generate_moves(Board& board, std::vector<Move>& moves) {
    moves.clear();
    moves.reserve(256);
    
    int side = board.side;
    bool is_white_turn = (side == WHITE);
    
    for (int sq = 0; sq < 120; sq++) {
        int p = board.pieces[sq];
        if (p == EMPTY || p == OFFBOARD) continue;
        if ((is_white_turn && p < 0) || (!is_white_turn && p > 0)) continue;
        
        int abs_p = std::abs(p);
        switch(abs_p) {
            case W_PAWN: generate_pawn_moves(board, moves, sq, side); break;
            case W_KNIGHT: generate_knight_moves(board, moves, sq, side); break;
            case W_BISHOP: generate_sliding_moves(board, moves, sq, side, BISHOP_OFFSETS, 4); break;
            case W_ROOK: generate_sliding_moves(board, moves, sq, side, ROOK_OFFSETS, 4); break;
            case W_QUEEN: 
                generate_sliding_moves(board, moves, sq, side, BISHOP_OFFSETS, 4);
                generate_sliding_moves(board, moves, sq, side, ROOK_OFFSETS, 4);
                break;
            case W_KING: generate_king_moves(board, moves, sq, side); break;
        }
    }
}

void generate_captures(Board& board, std::vector<Move>& moves) {
    moves.clear();
    moves.reserve(64);
    
    int side = board.side;
    bool is_white_turn = (side == WHITE);
    
    for (int sq = 0; sq < 120; sq++) {
        int p = board.pieces[sq];
        if (p == EMPTY || p == OFFBOARD) continue;
        if ((is_white_turn && p < 0) || (!is_white_turn && p > 0)) continue;
        
        int abs_p = std::abs(p);
        if (abs_p == W_PAWN) {
            int dir = is_white_turn ? 10 : -10;
            int promo_rank = is_white_turn ? 9 : 2;
            
            int to = sq + dir;
            if (board.pieces[to] == EMPTY && (to / 10 == promo_rank)) {
                moves.push_back(encode_move(sq, to, FLAG_PROMOTION));
            }
            
            int cap1 = sq + dir - 1;
            int cap2 = sq + dir + 1;
            
            if (board.pieces[cap1] != OFFBOARD) {
                if (is_white_turn && board.pieces[cap1] < 0) {
                    if (cap1 / 10 == promo_rank) moves.push_back(encode_move(sq, cap1, FLAG_PROMOTION));
                    else moves.push_back(encode_move(sq, cap1, FLAG_CAPTURE));
                } else if (!is_white_turn && board.pieces[cap1] > 0) {
                    if (cap1 / 10 == promo_rank) moves.push_back(encode_move(sq, cap1, FLAG_PROMOTION));
                    else moves.push_back(encode_move(sq, cap1, FLAG_CAPTURE));
                } else if (cap1 == board.en_passant) {
                    moves.push_back(encode_move(sq, cap1, FLAG_NONE));
                }
            }
            if (board.pieces[cap2] != OFFBOARD) {
                if (is_white_turn && board.pieces[cap2] < 0) {
                    if (cap2 / 10 == promo_rank) moves.push_back(encode_move(sq, cap2, FLAG_PROMOTION));
                    else moves.push_back(encode_move(sq, cap2, FLAG_CAPTURE));
                } else if (!is_white_turn && board.pieces[cap2] > 0) {
                    if (cap2 / 10 == promo_rank) moves.push_back(encode_move(sq, cap2, FLAG_PROMOTION));
                    else moves.push_back(encode_move(sq, cap2, FLAG_CAPTURE));
                } else if (cap2 == board.en_passant) {
                    moves.push_back(encode_move(sq, cap2, FLAG_NONE));
                }
            }
        } else if (abs_p == W_KNIGHT) {
            for (int offset : KNIGHT_OFFSETS) {
                int to_sq = sq + offset;
                int target_p = board.pieces[to_sq];
                if (target_p == OFFBOARD) continue;
                if (is_white_turn && target_p < 0) {
                    moves.push_back(encode_move(sq, to_sq, FLAG_CAPTURE));
                } else if (!is_white_turn && target_p > 0) {
                    moves.push_back(encode_move(sq, to_sq, FLAG_CAPTURE));
                }
            }
        } else if (abs_p == W_KING) {
            for (int offset : KING_OFFSETS) {
                int to_sq = sq + offset;
                int target_p = board.pieces[to_sq];
                if (target_p == OFFBOARD) continue;
                if (is_white_turn && target_p < 0) {
                    moves.push_back(encode_move(sq, to_sq, FLAG_CAPTURE));
                } else if (!is_white_turn && target_p > 0) {
                    moves.push_back(encode_move(sq, to_sq, FLAG_CAPTURE));
                }
            }
        } else if (abs_p == W_BISHOP) {
            for (int offset : BISHOP_OFFSETS) {
                int to_sq = sq + offset;
                while (board.pieces[to_sq] != OFFBOARD) {
                    int target_p = board.pieces[to_sq];
                    if (target_p == EMPTY) {
                        to_sq += offset;
                    } else {
                        if (is_white_turn && target_p < 0) moves.push_back(encode_move(sq, to_sq, FLAG_CAPTURE));
                        else if (!is_white_turn && target_p > 0) moves.push_back(encode_move(sq, to_sq, FLAG_CAPTURE));
                        break;
                    }
                }
            }
        } else if (abs_p == W_ROOK) {
            for (int offset : ROOK_OFFSETS) {
                int to_sq = sq + offset;
                while (board.pieces[to_sq] != OFFBOARD) {
                    int target_p = board.pieces[to_sq];
                    if (target_p == EMPTY) {
                        to_sq += offset;
                    } else {
                        if (is_white_turn && target_p < 0) moves.push_back(encode_move(sq, to_sq, FLAG_CAPTURE));
                        else if (!is_white_turn && target_p > 0) moves.push_back(encode_move(sq, to_sq, FLAG_CAPTURE));
                        break;
                    }
                }
            }
        } else if (abs_p == W_QUEEN) {
            for (int offset : BISHOP_OFFSETS) {
                int to_sq = sq + offset;
                while (board.pieces[to_sq] != OFFBOARD) {
                    int target_p = board.pieces[to_sq];
                    if (target_p == EMPTY) {
                        to_sq += offset;
                    } else {
                        if (is_white_turn && target_p < 0) moves.push_back(encode_move(sq, to_sq, FLAG_CAPTURE));
                        else if (!is_white_turn && target_p > 0) moves.push_back(encode_move(sq, to_sq, FLAG_CAPTURE));
                        break;
                    }
                }
            }
            for (int offset : ROOK_OFFSETS) {
                int to_sq = sq + offset;
                while (board.pieces[to_sq] != OFFBOARD) {
                    int target_p = board.pieces[to_sq];
                    if (target_p == EMPTY) {
                        to_sq += offset;
                    } else {
                        if (is_white_turn && target_p < 0) moves.push_back(encode_move(sq, to_sq, FLAG_CAPTURE));
                        else if (!is_white_turn && target_p > 0) moves.push_back(encode_move(sq, to_sq, FLAG_CAPTURE));
                        break;
                    }
                }
            }
        }
    }
}
