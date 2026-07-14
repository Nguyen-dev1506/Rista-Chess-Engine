#include "board.h"
#include "magic.h"
#include <iostream>
#include <sstream>

Board::Board() {
    set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void Board::compute_hash() {
    hash_key = 0;
    for (int p = 0; p < 12; p++) {
        U64 bb = pieces[p];
        while (bb) {
            int sq = pop_lsb(bb);
            hash_key ^= Zobrist::piece_keys[p][sq];
        }
    }
    if (side_to_move == BLACK) hash_key ^= Zobrist::side_key;
    if (en_passant != NO_SQ) {
        // Only include the en-passant key if a pawn of the side to move
        // can actually capture en passant (matches Book::compute_polyglot_hash).
        U64 ep_pawns = pieces[(side_to_move == WHITE) ? W_PAWN : B_PAWN];
        U64 cap_left = (side_to_move == WHITE) ? ((ep_pawns << 7) & 0x7F7F7F7F7F7F7F7FULL) : ((ep_pawns >> 9) & 0x7F7F7F7F7F7F7F7FULL);
        U64 cap_right = (side_to_move == WHITE) ? ((ep_pawns << 9) & 0xFEFEFEFEFEFEFEFEULL) : ((ep_pawns >> 7) & 0xFEFEFEFEFEFEFEFEULL);
        if ((cap_left | cap_right) & (1ULL << en_passant)) {
            hash_key ^= Zobrist::enpassant_keys[en_passant % 8];
        }
    }
    hash_key ^= Zobrist::castle_keys[castle_rights];
}

void Board::update_hash_piece(Piece p, Square sq) {
    hash_key ^= Zobrist::piece_keys[p][sq];
}

void Board::set_fen(const std::string& fen) {
    for (int i = 0; i < 12; i++) pieces[i] = 0;
    colors[0] = colors[1] = colors[2] = 0;
    en_passant = NO_SQ;
    castle_rights = 0;
    half_moves = 0;
    full_moves = 1;
    state_ply = 0;

    std::istringstream iss(fen);
    std::string board_part, side, castling, ep, half, full;
    iss >> board_part >> side >> castling >> ep >> half >> full;

    int r = 7, c = 0;
    for (char ch : board_part) {
        if (ch == '/') { r--; c = 0; }
        else if (isdigit(ch)) { c += ch - '0'; }
        else {
            if (r >= 0 && r <= 7 && c >= 0 && c <= 7) {
                Square sq = static_cast<Square>(r * 8 + c);
                Piece p = EMPTY_PIECE;
                switch (ch) {
                    case 'P': p = W_PAWN; break; case 'N': p = W_KNIGHT; break;
                    case 'B': p = W_BISHOP; break; case 'R': p = W_ROOK; break;
                    case 'Q': p = W_QUEEN; break; case 'K': p = W_KING; break;
                    case 'p': p = B_PAWN; break; case 'n': p = B_KNIGHT; break;
                    case 'b': p = B_BISHOP; break; case 'r': p = B_ROOK; break;
                    case 'q': p = B_QUEEN; break; case 'k': p = B_KING; break;
                }
                if (p != EMPTY_PIECE) {
                    set_bit(pieces[p], sq);
                    set_bit(colors[piece_color(p)], sq);
                    set_bit(colors[BOTH], sq);
                }
            }
            c++;
        }
    }

    side_to_move = (side == "w") ? WHITE : BLACK;

    for (char ch : castling) {
        if (ch == 'K') castle_rights |= WK;
        if (ch == 'Q') castle_rights |= WQ;
        if (ch == 'k') castle_rights |= BK;
        if (ch == 'q') castle_rights |= BQ;
    }

    if (ep != "-" && ep.size() >= 2) {
        int f = ep[0] - 'a';
        int r_ep = ep[1] - '1';
        en_passant = static_cast<Square>(r_ep * 8 + f);
    }

    if (!half.empty()) {
        try {
            half_moves = std::stoi(half);
        } catch (const std::exception&) {
            half_moves = 0;
        }
    }
    if (!full.empty()) {
        try {
            full_moves = std::stoi(full);
        } catch (const std::exception&) {
            full_moves = 1;
        }
    }
    
    compute_hash();
}

bool Board::is_attacked(Square sq, Color attacker) const {
    if (attacker == WHITE) {
        if (Bitboards::PawnAttacks[BLACK][sq] & pieces[W_PAWN]) return true;
        if (Bitboards::KnightAttacks[sq] & pieces[W_KNIGHT]) return true;
        if (Bitboards::KingAttacks[sq] & pieces[W_KING]) return true;
        U64 b_att = Magic::get_bishop_attacks(sq, colors[BOTH]);
        if (b_att & (pieces[W_BISHOP] | pieces[W_QUEEN])) return true;
        U64 r_att = Magic::get_rook_attacks(sq, colors[BOTH]);
        if (r_att & (pieces[W_ROOK] | pieces[W_QUEEN])) return true;
    } else {
        if (Bitboards::PawnAttacks[WHITE][sq] & pieces[B_PAWN]) return true;
        if (Bitboards::KnightAttacks[sq] & pieces[B_KNIGHT]) return true;
        if (Bitboards::KingAttacks[sq] & pieces[B_KING]) return true;
        U64 b_att = Magic::get_bishop_attacks(sq, colors[BOTH]);
        if (b_att & (pieces[B_BISHOP] | pieces[B_QUEEN])) return true;
        U64 r_att = Magic::get_rook_attacks(sq, colors[BOTH]);
        if (r_att & (pieces[B_ROOK] | pieces[B_QUEEN])) return true;
    }
    return false;
}

bool Board::is_in_check(Color c) const {
    U64 king_bb = pieces[c == WHITE ? W_KING : B_KING];
    if (!king_bb) return false;
    return is_attacked(static_cast<Square>(lsb(king_bb)), c == WHITE ? BLACK : WHITE);
}

void Board::make_move(Move m) {
    // Clamp into range instead of writing out of bounds in the (extremely
    // rare) case a game/search line exceeds state_history's capacity.
    int hist_idx = (state_ply < 1024) ? state_ply : 1023;
    state_history[hist_idx] = {en_passant, castle_rights, half_moves, hash_key, EMPTY_PIECE};
    
    Square from = m.from();
    Square to = m.to();
    uint16_t flags = m.flags();
    
    Piece p = piece_on(from);
    Piece captured = piece_on(to);
    Color us = side_to_move;
    Color them = (us == WHITE) ? BLACK : WHITE;

    // Precompute (before any piece mutation) whether the existing en-passant
    // square could actually be captured by "us" -- matches the same
    // condition used when this key was originally added to the hash.
    bool old_ep_capturable = false;
    if (en_passant != NO_SQ) {
        U64 ep_pawns = pieces[(us == WHITE) ? W_PAWN : B_PAWN];
        U64 cap_left = (us == WHITE) ? ((ep_pawns << 7) & 0x7F7F7F7F7F7F7F7FULL) : ((ep_pawns >> 9) & 0x7F7F7F7F7F7F7F7FULL);
        U64 cap_right = (us == WHITE) ? ((ep_pawns << 9) & 0xFEFEFEFEFEFEFEFEULL) : ((ep_pawns >> 7) & 0xFEFEFEFEFEFEFEFEULL);
        old_ep_capturable = (cap_left | cap_right) & (1ULL << en_passant);
    }

    state_history[hist_idx].captured = captured;
    
    // Remove piece from source
    clear_bit(pieces[p], from);
    clear_bit(colors[us], from);
    clear_bit(colors[BOTH], from);
    update_hash_piece(p, from);
    
    // Handle capture
    if (captured != EMPTY_PIECE && flags != EP_CAPTURE) {
        clear_bit(pieces[captured], to);
        clear_bit(colors[them], to);
        clear_bit(colors[BOTH], to);
        update_hash_piece(captured, to);
    }
    
    // En-passant capture
    if (flags == EP_CAPTURE) {
        Square cap_sq = (us == WHITE) ? static_cast<Square>(to - 8) : static_cast<Square>(to + 8);
        Piece cap_p = (us == WHITE) ? B_PAWN : W_PAWN;
        clear_bit(pieces[cap_p], cap_sq);
        clear_bit(colors[them], cap_sq);
        clear_bit(colors[BOTH], cap_sq);
        update_hash_piece(cap_p, cap_sq);
    }
    
    // Handle Promotion
    Piece place_piece = p;
    if (flags >= N_PROMO && flags <= Q_PROMO_CAP) {
        int promo_type = (flags & 3);
        place_piece = static_cast<Piece>((us == WHITE ? W_KNIGHT : B_KNIGHT) + promo_type);
    }
    
    // Place piece at destination
    set_bit(pieces[place_piece], to);
    set_bit(colors[us], to);
    set_bit(colors[BOTH], to);
    update_hash_piece(place_piece, to);
    
    // Castling
    if (flags == KING_CASTLE) {
        Square r_from = (us == WHITE) ? H1 : H8;
        Square r_to = (us == WHITE) ? F1 : F8;
        Piece r = (us == WHITE) ? W_ROOK : B_ROOK;
        clear_bit(pieces[r], r_from); clear_bit(colors[us], r_from); clear_bit(colors[BOTH], r_from);
        set_bit(pieces[r], r_to); set_bit(colors[us], r_to); set_bit(colors[BOTH], r_to);
        update_hash_piece(r, r_from); update_hash_piece(r, r_to);
    } else if (flags == QUEEN_CASTLE) {
        Square r_from = (us == WHITE) ? A1 : A8;
        Square r_to = (us == WHITE) ? D1 : D8;
        Piece r = (us == WHITE) ? W_ROOK : B_ROOK;
        clear_bit(pieces[r], r_from); clear_bit(colors[us], r_from); clear_bit(colors[BOTH], r_from);
        set_bit(pieces[r], r_to); set_bit(colors[us], r_to); set_bit(colors[BOTH], r_to);
        update_hash_piece(r, r_from); update_hash_piece(r, r_to);
    }
    
    // Update hash for ep and castle
    if (en_passant != NO_SQ && old_ep_capturable) hash_key ^= Zobrist::enpassant_keys[en_passant % 8];
    hash_key ^= Zobrist::castle_keys[castle_rights];

    en_passant = NO_SQ;
    if (flags == DOUBLE_PAWN) {
        en_passant = (us == WHITE) ? static_cast<Square>(to - 8) : static_cast<Square>(to + 8);
        U64 new_ep_pawns = pieces[(them == WHITE) ? W_PAWN : B_PAWN];
        U64 new_cap_left = (them == WHITE) ? ((new_ep_pawns << 7) & 0x7F7F7F7F7F7F7F7FULL) : ((new_ep_pawns >> 9) & 0x7F7F7F7F7F7F7F7FULL);
        U64 new_cap_right = (them == WHITE) ? ((new_ep_pawns << 9) & 0xFEFEFEFEFEFEFEFEULL) : ((new_ep_pawns >> 7) & 0xFEFEFEFEFEFEFEFEULL);
        if ((new_cap_left | new_cap_right) & (1ULL << en_passant)) {
            hash_key ^= Zobrist::enpassant_keys[en_passant % 8];
        }
    }
    
    // Update castling rights
    const int castling_rights_update[64] = {
        13, 15, 15, 15, 12, 15, 15, 14,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
         7, 15, 15, 15,  3, 15, 15, 11
    };
    castle_rights &= castling_rights_update[from];
    castle_rights &= castling_rights_update[to];
    hash_key ^= Zobrist::castle_keys[castle_rights];
    
    hash_key ^= Zobrist::side_key;
    side_to_move = them;
    
    if (piece_type(p) == PAWN || captured != EMPTY_PIECE) half_moves = 0;
    else half_moves++;
    
    if (us == BLACK) full_moves++;
    state_ply++;
}

void Board::unmake_move(Move m) {
    state_ply--;
    int hist_idx = (state_ply < 1024) ? state_ply : 1023;
    State state = state_history[hist_idx];
    
    Square from = m.from();
    Square to = m.to();
    uint16_t flags = m.flags();
    
    Color them = side_to_move;
    Color us = (them == WHITE) ? BLACK : WHITE;
    
    Piece captured = state.captured;
    Piece p = piece_on(to); // It's currently at 'to'
    
    // Reverse Promotion
    if (flags >= N_PROMO && flags <= Q_PROMO_CAP) {
        clear_bit(pieces[p], to);
        p = (us == WHITE) ? W_PAWN : B_PAWN;
        set_bit(pieces[p], to); // temporary, will be moved
    }
    
    // Move piece back
    clear_bit(pieces[p], to);
    clear_bit(colors[us], to);
    clear_bit(colors[BOTH], to);
    
    set_bit(pieces[p], from);
    set_bit(colors[us], from);
    set_bit(colors[BOTH], from);
    
    // Restore capture
    if (captured != EMPTY_PIECE && flags != EP_CAPTURE) {
        set_bit(pieces[captured], to);
        set_bit(colors[them], to);
        set_bit(colors[BOTH], to);
    }
    
    // Restore EP capture
    if (flags == EP_CAPTURE) {
        Square cap_sq = (us == WHITE) ? static_cast<Square>(to - 8) : static_cast<Square>(to + 8);
        Piece cap_p = (us == WHITE) ? B_PAWN : W_PAWN;
        set_bit(pieces[cap_p], cap_sq);
        set_bit(colors[them], cap_sq);
        set_bit(colors[BOTH], cap_sq);
    }
    
    // Reverse Castling
    if (flags == KING_CASTLE) {
        Square r_from = (us == WHITE) ? H1 : H8;
        Square r_to = (us == WHITE) ? F1 : F8;
        Piece r = (us == WHITE) ? W_ROOK : B_ROOK;
        clear_bit(pieces[r], r_to); clear_bit(colors[us], r_to); clear_bit(colors[BOTH], r_to);
        set_bit(pieces[r], r_from); set_bit(colors[us], r_from); set_bit(colors[BOTH], r_from);
    } else if (flags == QUEEN_CASTLE) {
        Square r_from = (us == WHITE) ? A1 : A8;
        Square r_to = (us == WHITE) ? D1 : D8;
        Piece r = (us == WHITE) ? W_ROOK : B_ROOK;
        clear_bit(pieces[r], r_to); clear_bit(colors[us], r_to); clear_bit(colors[BOTH], r_to);
        set_bit(pieces[r], r_from); set_bit(colors[us], r_from); set_bit(colors[BOTH], r_from);
    }
    
    // Restore state
    en_passant = state.ep;
    castle_rights = state.castle;
    half_moves = state.half_moves;
    hash_key = state.hash;
    
    side_to_move = us;
    if (us == BLACK) full_moves--;
}

void Board::print() const {
    std::cout << "\n";
    for (int r = 7; r >= 0; r--) {
        std::cout << r + 1 << "  ";
        for (int c = 0; c < 8; c++) {
            Square sq = static_cast<Square>(r * 8 + c);
            Piece p = piece_on(sq);
            char ch = '.';
            switch (p) {
                case W_PAWN: ch = 'P'; break; case W_KNIGHT: ch = 'N'; break;
                case W_BISHOP: ch = 'B'; break; case W_ROOK: ch = 'R'; break;
                case W_QUEEN: ch = 'Q'; break; case W_KING: ch = 'K'; break;
                case B_PAWN: ch = 'p'; break; case B_KNIGHT: ch = 'n'; break;
                case B_BISHOP: ch = 'b'; break; case B_ROOK: ch = 'r'; break;
                case B_QUEEN: ch = 'q'; break; case B_KING: ch = 'k'; break;
                default: break;
            }
            std::cout << ch << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n   a b c d e f g h\n";
    std::cout << "Side: " << (side_to_move == WHITE ? "White" : "Black") << "\n";
}
