#include "board.h"
#include "zobrist.h"
#include <iostream>
#include <sstream>
#include <cctype>
#include <cmath>
#include <unordered_map>

// Piece values (Centipawns)
const int PIECE_VALUES[] = {
    0, 100, 320, 330, 500, 900, 20000, // EMPTY, P, N, B, R, Q, K
};

// PST (Piece-Square Tables) - Visual representation (Rank 8 at top, Rank 1 at bottom)
// We will map sq64 to visual_sq in get_pst_value to access these correctly.

const int PST_PAWN[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
     50, 50, 50, 50, 50, 50, 50, 50,
     10, 10, 20, 30, 30, 20, 10, 10,
      5,  5, 10, 25, 25, 10,  5,  5,
    -20,-20,-20, 20, 20,-20,-20,-20, // Penalize flank, reward central (d4, e4)
    -20,-20,-20, 10, 10,-20,-20,-20, // Penalize flank, reward central (d3, e3)
      0,  0,  0,-20,-20,  0,  0,  0, // Start pos, central pawns want to push
      0,  0,  0,  0,  0,  0,  0,  0
};

const int PST_KNIGHT[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

const int PST_BISHOP[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

const int PST_ROOK[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
      5, 10, 10, 10, 10, 10, 10,  5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
      0,  0,  0,  5,  5,  0,  0,  0
};

const int PST_QUEEN[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

const int PST_KING_MG[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

// Map square index for flip
int mirror_sq(int sq120) {
    int sq64 = sq120_to_sq64(sq120);
    return sq64_to_sq120(sq64 ^ 56);
}

int get_piece_value(int piece) {
    if (piece == EMPTY || piece == OFFBOARD) return 0;
    return piece > 0 ? PIECE_VALUES[piece] : -PIECE_VALUES[-piece];
}

int get_pst_value(int piece, int sq) {
    if (piece == EMPTY || piece == OFFBOARD) return 0;
    
    int sq64 = sq120_to_sq64(piece > 0 ? sq : mirror_sq(sq)); // Mirror for Black
    // Map sq64 (0 at A1) to visual index (0 at A8)
    int visual_sq = (7 - (sq64 / 8)) * 8 + (sq64 % 8);
    int val = 0;
    
    int abs_piece = std::abs(piece);
    switch(abs_piece) {
        case W_PAWN: val = PST_PAWN[visual_sq]; break;
        case W_KNIGHT: val = PST_KNIGHT[visual_sq]; break;
        case W_BISHOP: val = PST_BISHOP[visual_sq]; break;
        case W_ROOK: val = PST_ROOK[visual_sq]; break;
        case W_QUEEN: val = PST_QUEEN[visual_sq]; break;
        case W_KING: val = PST_KING_MG[visual_sq]; break;
    }
    
    return piece > 0 ? val : -val;
}


uint64_t Board::generate_pos_key() {
    uint64_t final_key = 0;
    for (int sq = 0; sq < 120; sq++) {
        int piece = pieces[sq];
        if (piece != OFFBOARD && piece != EMPTY) {
            final_key ^= piece_keys[get_piece_index(piece)][sq];
        }
    }
    if (side == BLACK) {
        final_key ^= side_key;
    }
    if (en_passant != SQ_NONE) {
        final_key ^= ep_keys[en_passant];
    }
    final_key ^= castling_keys[castling_rights];
    return final_key;
}

Board::Board() {
    reset();
}

void Board::reset() {
    for (int i = 0; i < 120; i++) pieces[i] = OFFBOARD;
    for (int r = 0; r < 8; r++) {
        for (int f = 0; f < 8; f++) {
            pieces[sq64_to_sq120(r * 8 + f)] = EMPTY;
        }
    }
    
    side = WHITE;
    en_passant = SQ_NONE;
    castling_rights = 0;
    fifty_move = 0;
    material_score = 0;
    pst_score = 0;
    history.clear();
    hash_key = 0;
}

void Board::set_fen(const std::string& fen) {
    reset();
    
    std::istringstream iss(fen);
    std::string board_part, side_part, castling_part, enpassant_part, halfmove_part, fullmove_part;
    iss >> board_part >> side_part >> castling_part >> enpassant_part >> halfmove_part >> fullmove_part;
    
    int rank = 7, file = 0;
    for (char c : board_part) {
        if (c == '/') {
            rank--;
            file = 0;
        } else if (std::isdigit(c)) {
            file += c - '0';
        } else {
            int sq = sq64_to_sq120(rank * 8 + file);
            int p = EMPTY;
            switch(c) {
                case 'P': p = W_PAWN; break;
                case 'N': p = W_KNIGHT; break;
                case 'B': p = W_BISHOP; break;
                case 'R': p = W_ROOK; break;
                case 'Q': p = W_QUEEN; break;
                case 'K': p = W_KING; break;
                case 'p': p = B_PAWN; break;
                case 'n': p = B_KNIGHT; break;
                case 'b': p = B_BISHOP; break;
                case 'r': p = B_ROOK; break;
                case 'q': p = B_QUEEN; break;
                case 'k': p = B_KING; break;
            }
            pieces[sq] = p;
            update_pst_score(p, sq, true);
            file++;
        }
    }
    
    side = (side_part == "b") ? BLACK : WHITE;
    
    castling_rights = 0;
    if (castling_part != "-") {
        if (castling_part.find('K') != std::string::npos) castling_rights |= WK_CASTLING;
        if (castling_part.find('Q') != std::string::npos) castling_rights |= WQ_CASTLING;
        if (castling_part.find('k') != std::string::npos) castling_rights |= BK_CASTLING;
        if (castling_part.find('q') != std::string::npos) castling_rights |= BQ_CASTLING;
    }
    
    en_passant = SQ_NONE;
    if (enpassant_part != "-") {
        int f = enpassant_part[0] - 'a';
        int r = enpassant_part[1] - '1';
        en_passant = sq64_to_sq120(r * 8 + f);
    }
    
    if (!halfmove_part.empty()) fifty_move = std::stoi(halfmove_part);
    hash_key = generate_pos_key();
}

void Board::update_pst_score(int piece, int sq, bool is_add) {
    int mat = get_piece_value(piece);
    int pst = get_pst_value(piece, sq);
    if (is_add) {
        material_score += mat;
        pst_score += pst;
    } else {
        material_score -= mat;
        pst_score -= pst;
    }
    hash_key ^= piece_keys[get_piece_index(piece)][sq];
}

bool Board::make_move(Move m) {
    int from = move_from(m);
    int to = move_to(m);
    int flag = move_flag(m);
    
    int piece = pieces[from];
    int captured = pieces[to];
    
    UndoMove undo;
    undo.move = m;
    undo.captured = captured;
    undo.en_passant = en_passant;
    undo.castling_rights = castling_rights;
    undo.fifty_move = fifty_move;
    undo.hash_key = hash_key;
    
    if (en_passant != SQ_NONE) hash_key ^= ep_keys[en_passant];
    hash_key ^= castling_keys[castling_rights];
    
    history.push_back(undo);
    
    // Remove pieces from PST
    update_pst_score(piece, from, false);
    if (captured != EMPTY) {
        update_pst_score(captured, to, false);
    }
    
    // Move piece
    pieces[to] = piece;
    pieces[from] = EMPTY;
    
    // En Passant capture
    if (std::abs(piece) == W_PAWN && to == en_passant && flag == FLAG_NONE) {
        // En passant is captured
        int ep_pawn_sq = (side == WHITE) ? to - 10 : to + 10;
        undo.captured = pieces[ep_pawn_sq]; // We modify history to record the capture
        history.back().captured = undo.captured;
        
        update_pst_score(pieces[ep_pawn_sq], ep_pawn_sq, false);
        pieces[ep_pawn_sq] = EMPTY;
    }
    
    // Promotion
    if (flag == FLAG_PROMOTION) {
        piece = (side == WHITE) ? W_QUEEN : B_QUEEN; // auto queen promotion for now
        pieces[to] = piece;
    }
    
    // Castling
    if (flag == FLAG_CASTLING) {
        if (to == G1) { pieces[F1] = W_ROOK; pieces[H1] = EMPTY; update_pst_score(W_ROOK, H1, false); update_pst_score(W_ROOK, F1, true); } // wk
        else if (to == C1) { pieces[D1] = W_ROOK; pieces[A1] = EMPTY; update_pst_score(W_ROOK, A1, false); update_pst_score(W_ROOK, D1, true); } // wq
        else if (to == G8) { pieces[F8] = B_ROOK; pieces[H8] = EMPTY; update_pst_score(B_ROOK, H8, false); update_pst_score(B_ROOK, F8, true); } // bk
        else if (to == C8) { pieces[D8] = B_ROOK; pieces[A8] = EMPTY; update_pst_score(B_ROOK, A8, false); update_pst_score(B_ROOK, D8, true); } // bq
    }
    
    // Add piece back to PST
    update_pst_score(piece, to, true);
    
    // Update En Passant square
    en_passant = SQ_NONE;
    if (std::abs(piece) == W_PAWN && std::abs(from - to) == 20) {
        en_passant = (side == WHITE) ? from + 10 : from - 10;
        hash_key ^= ep_keys[en_passant];
    }
    
    // Update Castling rights
    if (piece == W_KING) { castling_rights &= ~(WK_CASTLING | WQ_CASTLING); }
    if (piece == B_KING) { castling_rights &= ~(BK_CASTLING | BQ_CASTLING); }
    
    if (from == H1 || to == H1) castling_rights &= ~WK_CASTLING;
    if (from == A1 || to == A1) castling_rights &= ~WQ_CASTLING;
    if (from == H8 || to == H8) castling_rights &= ~BK_CASTLING;
    if (from == A8 || to == A8) castling_rights &= ~BQ_CASTLING;
    
    hash_key ^= castling_keys[castling_rights];
    hash_key ^= side_key; // side switches
    
    fifty_move++;
    if (captured != EMPTY || std::abs(piece) == W_PAWN) fifty_move = 0;
    
    side ^= 1; // switch side
    
    if (is_in_check(side ^ 1)) {
        unmake_move();
        return false;
    }
    
    return true;
}

void Board::unmake_move() {
    if (history.empty()) return;
    
    side ^= 1;
    
    UndoMove undo = history.back();
    history.pop_back();
    hash_key = undo.hash_key;
    
    Move m = undo.move;
    int from = move_from(m);
    int to = move_to(m);
    int flag = move_flag(m);
    
    int piece = pieces[to];
    
    // Reverse Promotion
    if (flag == FLAG_PROMOTION) {
        update_pst_score(piece, to, false);
        piece = (side == WHITE) ? W_PAWN : B_PAWN;
        pieces[to] = piece;
        update_pst_score(piece, to, true);
    }
    
    // Reverse Castling
    if (flag == FLAG_CASTLING) {
        if (to == G1) { pieces[H1] = W_ROOK; pieces[F1] = EMPTY; update_pst_score(W_ROOK, F1, false); update_pst_score(W_ROOK, H1, true); }
        else if (to == C1) { pieces[A1] = W_ROOK; pieces[D1] = EMPTY; update_pst_score(W_ROOK, D1, false); update_pst_score(W_ROOK, A1, true); }
        else if (to == G8) { pieces[H8] = B_ROOK; pieces[F8] = EMPTY; update_pst_score(B_ROOK, F8, false); update_pst_score(B_ROOK, H8, true); }
        else if (to == C8) { pieces[A8] = B_ROOK; pieces[D8] = EMPTY; update_pst_score(B_ROOK, D8, false); update_pst_score(B_ROOK, A8, true); }
    }
    
    // Move piece back
    update_pst_score(piece, to, false);
    pieces[from] = piece;
    pieces[to] = undo.captured;
    update_pst_score(piece, from, true);
    
    // Handle En Passant capture specifically
    if (std::abs(piece) == W_PAWN && to == undo.en_passant && flag == FLAG_NONE) {
        pieces[to] = EMPTY;
        int ep_pawn_sq = (side == WHITE) ? to - 10 : to + 10;
        pieces[ep_pawn_sq] = undo.captured; // restore pawn
        update_pst_score(undo.captured, ep_pawn_sq, true);
    } else if (undo.captured != EMPTY) {
        update_pst_score(undo.captured, to, true);
    }
    
    en_passant = undo.en_passant;
    castling_rights = undo.castling_rights;
    fifty_move = undo.fifty_move;
}

// Offsets for sliding pieces and knights
extern const int KNIGHT_OFFSETS[8] = {-21, -19, -12, -8, 8, 12, 19, 21};
extern const int BISHOP_OFFSETS[4] = {-11, -9, 9, 11};
extern const int ROOK_OFFSETS[4] = {-10, -1, 1, 10};
extern const int KING_OFFSETS[8] = {-11, -10, -9, -1, 1, 9, 10, 11};

bool Board::is_square_attacked(int sq, int attacker_side) {
    // Pawn attacks
    if (attacker_side == WHITE) {
        if (pieces[sq - 9] == W_PAWN || pieces[sq - 11] == W_PAWN) return true;
    } else {
        if (pieces[sq + 9] == B_PAWN || pieces[sq + 11] == B_PAWN) return true;
    }
    
    // Knight attacks
    for (int offset : KNIGHT_OFFSETS) {
        if (pieces[sq + offset] == (attacker_side == WHITE ? W_KNIGHT : B_KNIGHT)) return true;
    }
    
    // King attacks
    for (int offset : KING_OFFSETS) {
        if (pieces[sq + offset] == (attacker_side == WHITE ? W_KING : B_KING)) return true;
    }
    
    // Bishop / Queen attacks
    for (int offset : BISHOP_OFFSETS) {
        int target = sq + offset;
        while (pieces[target] != OFFBOARD) {
            int p = pieces[target];
            if (p != EMPTY) {
                if (p == (attacker_side == WHITE ? W_BISHOP : B_BISHOP) || 
                    p == (attacker_side == WHITE ? W_QUEEN : B_QUEEN)) return true;
                break;
            }
            target += offset;
        }
    }
    
    // Rook / Queen attacks
    for (int offset : ROOK_OFFSETS) {
        int target = sq + offset;
        while (pieces[target] != OFFBOARD) {
            int p = pieces[target];
            if (p != EMPTY) {
                if (p == (attacker_side == WHITE ? W_ROOK : B_ROOK) || 
                    p == (attacker_side == WHITE ? W_QUEEN : B_QUEEN)) return true;
                break;
            }
            target += offset;
        }
    }
    
    return false;
}

bool Board::is_in_check(int side_to_check) {
    int king_sq = SQ_NONE;
    int king_piece = (side_to_check == WHITE) ? W_KING : B_KING;
    for (int i = 0; i < 120; i++) {
        if (pieces[i] == king_piece) {
            king_sq = i;
            break;
        }
    }
    return is_square_attacked(king_sq, side_to_check ^ 1);
}

Move Board::parse_move(const std::string& move_str) {
    if (move_str.length() < 4) return 0;
    
    int from_f = move_str[0] - 'a';
    int from_r = move_str[1] - '1';
    int to_f = move_str[2] - 'a';
    int to_r = move_str[3] - '1';
    
    int from = (from_r + 2) * 10 + (from_f + 1);
    int to = (to_r + 2) * 10 + (to_f + 1);
    
    int flag = FLAG_NONE;
    if (move_str.length() == 5) {
        // e.g., e7e8q
        flag = FLAG_PROMOTION;
    } else if (pieces[from] == W_KING && from == E1 && (to == G1 || to == C1)) {
        flag = FLAG_CASTLING;
    } else if (pieces[from] == B_KING && from == E8 && (to == G8 || to == C8)) {
        flag = FLAG_CASTLING;
    }
    // We could determine captures here, but FLAG_CAPTURE is mostly for move encoding optimization if needed.
    // We'll leave it as FLAG_NONE and let move generator handle full move generation.
    // Usually parse_move is just to create a matching 16-bit Move from UCI input.
    if (pieces[to] != EMPTY) flag = FLAG_CAPTURE; // Note: promotion can also be capture, but we only have 2 bits. In our engine, promotion takes precedence.
    if (move_str.length() == 5) flag = FLAG_PROMOTION;
    else if (flag != FLAG_CASTLING && pieces[to] != EMPTY) flag = FLAG_CAPTURE;
    
    return encode_move(from, to, flag);
}

std::string Board::move_to_string(Move m) {
    int from = move_from(m);
    int to = move_to(m);
    int flag = move_flag(m);
    
    int from_sq64 = sq120_to_sq64(from);
    int to_sq64 = sq120_to_sq64(to);
    
    std::string s = "";
    s += (char)('a' + (from_sq64 % 8));
    s += (char)('1' + (from_sq64 / 8));
    s += (char)('a' + (to_sq64 % 8));
    s += (char)('1' + (to_sq64 / 8));
    
    if (flag == FLAG_PROMOTION) {
        s += "q"; // Always queen promotion for now
    }
    
    return s;
}

std::string Board::get_book_move(const std::string& history) {
    static std::unordered_map<std::string, std::string> book = {
        {"", "e2e4"},
        {"e2e4 c7c5", "g1f3"},
        {"e2e4 c7c5 g1f3", "d7d6"}, // Sicilian Standard Response
        {"e2e4 c7c6", "d2d4"},
        {"e2e4 c7c6 d2d4", "d7d5"}, // Caro-Kann Standard Response
        {"e2e4 e7e5", "g1f3"},
        {"e2e4 e7e5 g1f3 b8c6", "f1c4"}
    };
    
    if (book.find(history) != book.end()) {
        return book[history];
    }
    return "";
}

void Board::print() {
    for (int r = 7; r >= 0; r--) {
        std::cout << r + 1 << "  ";
        for (int f = 0; f < 8; f++) {
            int p = pieces[sq64_to_sq120(r * 8 + f)];
            char c = '.';
            switch(p) {
                case W_PAWN: c = 'P'; break; case B_PAWN: c = 'p'; break;
                case W_KNIGHT: c = 'N'; break; case B_KNIGHT: c = 'n'; break;
                case W_BISHOP: c = 'B'; break; case B_BISHOP: c = 'b'; break;
                case W_ROOK: c = 'R'; break; case B_ROOK: c = 'r'; break;
                case W_QUEEN: c = 'Q'; break; case B_QUEEN: c = 'q'; break;
                case W_KING: c = 'K'; break; case B_KING: c = 'k'; break;
            }
            std::cout << c << " ";
        }
        std::cout << "\n";
    }
    std::cout << "   a b c d e f g h\n\n";
    std::cout << "Side: " << (side == WHITE ? "White" : "Black") << "\n";
}
