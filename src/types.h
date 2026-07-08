#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <string>
#include <cmath>

// Colors
enum Color { WHITE = 0, BLACK = 1, BOTH = 2 };

// Pieces
enum Piece {
    EMPTY = 0,
    W_PAWN = 1, W_KNIGHT = 2, W_BISHOP = 3, W_ROOK = 4, W_QUEEN = 5, W_KING = 6,
    B_PAWN = -1, B_KNIGHT = -2, B_BISHOP = -3, B_ROOK = -4, B_QUEEN = -5, B_KING = -6,
    OFFBOARD = 99
};

inline int piece_color(int piece) {
    if (piece > 0 && piece != OFFBOARD) return WHITE;
    if (piece < 0) return BLACK;
    return BOTH;
}

// Squares (10x12 Mailbox representation)
enum Square {
    A1 = 21, B1, C1, D1, E1, F1, G1, H1,
    A2 = 31, B2, C2, D2, E2, F2, G2, H2,
    A3 = 41, B3, C3, D3, E3, F3, G3, H3,
    A4 = 51, B4, C4, D4, E4, F4, G4, H4,
    A5 = 61, B5, C5, D5, E5, F5, G5, H5,
    A6 = 71, B6, C6, D6, E6, F6, G6, H6,
    A7 = 81, B7, C7, D7, E7, F7, G7, H7,
    A8 = 91, B8, C8, D8, E8, F8, G8, H8,
    SQ_NONE = 0
};

// Converting between 120 and 64 arrays if needed
inline int sq120_to_sq64(int sq120) {
    int file = (sq120 % 10) - 1;
    int rank = (sq120 / 10) - 2;
    return rank * 8 + file;
}

inline int sq64_to_sq120(int sq64) {
    int file = sq64 % 8;
    int rank = sq64 / 8;
    return (rank + 2) * 10 + (file + 1);
}

// Flags for move encoding
enum MoveFlag {
    FLAG_NONE = 0,
    FLAG_CAPTURE = 1,
    FLAG_PROMOTION = 2,
    FLAG_CASTLING = 3,
    FLAG_ENPASSANT = 1 // we can use capture flag or define more bits. Let's use 4 bits for flags just in case.
};

// 16-bit Move Encoding
// bit 0-6: from square (0-119) -> 7 bits
// bit 7-13: to square (0-119) -> 7 bits
// bit 14-15: flags (0-3) -> 2 bits
// Since en_passant is a capture, we can just use FLAG_CAPTURE and check for en_passant explicitly.
// Actually, using 4 bits for flags is better to distinguish ep, promotion piece, etc. 
// But let's stick to the prompt: "7 bit đầu cho ô đi (From), 7 bit tiếp theo cho ô đến (To), 2 bit cuối cho cờ hiệu Flags (Bình thường, Ăn quân, Phong cấp, Nhập thành)"
typedef uint16_t Move;

inline Move encode_move(int from, int to, int flag) {
    return (from & 0x7F) | ((to & 0x7F) << 7) | ((flag & 0x3) << 14);
}

inline int move_from(Move m) { return m & 0x7F; }
inline int move_to(Move m) { return (m >> 7) & 0x7F; }
inline int move_flag(Move m) { return (m >> 14) & 0x3; }

// Castling rights
enum CastlingRights {
    WK_CASTLING = 1,
    WQ_CASTLING = 2,
    BK_CASTLING = 4,
    BQ_CASTLING = 8
};

// Struct to store history for unmaking moves
struct UndoMove {
    Move move;
    int captured;
    int en_passant; // square for en_passant, or SQ_NONE
    int castling_rights;
    int fifty_move;
    uint64_t hash_key;
};

#endif
