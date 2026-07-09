#ifndef TYPES_H
#define TYPES_H

#include <cstdint>

typedef uint64_t U64;

enum Color { WHITE, BLACK, BOTH };
enum PieceType { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NONE };
enum Piece {
    W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    EMPTY_PIECE
};

enum Square {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    NO_SQ
};

enum CastleRight {
    WK = 1, WQ = 2, BK = 4, BQ = 8
};

// 16-bit Move representation
// bit 0-5: source square (0-63)
// bit 6-11: target square (0-63)
// bit 12-15: promoted piece or flags
struct Move {
    uint16_t move;
    Move() : move(0) {}
    Move(uint16_t m) : move(m) {}
    Move(Square from, Square to, uint16_t flags = 0) {
        move = (from & 0x3F) | ((to & 0x3F) << 6) | (flags << 12);
    }
    Square from() const { return static_cast<Square>(move & 0x3F); }
    Square to() const { return static_cast<Square>((move >> 6) & 0x3F); }
    uint16_t flags() const { return (move >> 12) & 0xF; }
    
    bool is_valid() const { return move != 0; }
    bool operator==(const Move& other) const { return move == other.move; }
};

// Move flags
constexpr uint16_t QUIET = 0;
constexpr uint16_t DOUBLE_PAWN = 1;
constexpr uint16_t KING_CASTLE = 2;
constexpr uint16_t QUEEN_CASTLE = 3;
constexpr uint16_t CAPTURE = 4;
constexpr uint16_t EP_CAPTURE = 5;
constexpr uint16_t N_PROMO = 8;
constexpr uint16_t B_PROMO = 9;
constexpr uint16_t R_PROMO = 10;
constexpr uint16_t Q_PROMO = 11;
constexpr uint16_t N_PROMO_CAP = 12;
constexpr uint16_t B_PROMO_CAP = 13;
constexpr uint16_t R_PROMO_CAP = 14;
constexpr uint16_t Q_PROMO_CAP = 15;

inline Color piece_color(Piece p) { return p < B_PAWN ? WHITE : BLACK; }
inline PieceType piece_type(Piece p) { return static_cast<PieceType>(p % 6); }

#endif
