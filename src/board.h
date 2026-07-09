#ifndef BOARD_H
#define BOARD_H

#include "types.h"
#include "zobrist.h"
#include "bitboard.h"
#include <array>
#include <string>

class Board {
public:
    Board();
    void set_fen(const std::string& fen);
    void print() const;
    
    // Core bitboards
    std::array<U64, 12> pieces;
    U64 colors[3]; // WHITE, BLACK, BOTH
    
    Color side_to_move;
    Square en_passant;
    int castle_rights;
    int half_moves;
    int full_moves;
    
    U64 hash_key;
    
    inline U64 occ() const { return colors[BOTH]; }
    inline U64 occ(Color c) const { return colors[c]; }
    inline Piece piece_on(Square sq) const {
        for (int p = 0; p < 12; p++) {
            if (get_bit(pieces[p], sq)) return static_cast<Piece>(p);
        }
        return EMPTY_PIECE;
    }

    void make_move(Move m);
    void unmake_move(Move m);
    
    struct State {
        Square ep;
        int castle;
        int half_moves;
        U64 hash;
        Piece captured;
    };
    
    std::array<State, 1024> state_history;
    int state_ply;
    
    void compute_hash();
    void update_hash_piece(Piece p, Square sq);
    
    bool is_in_check(Color c) const;
    bool is_attacked(Square sq, Color attacker) const;
};

#endif
