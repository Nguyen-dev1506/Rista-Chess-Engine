#ifndef BOARD_H
#define BOARD_H

#include "types.h"
#include <vector>
#include <string>

class Board {
public:
    int pieces[120];
    int side;
    int en_passant;
    int castling_rights;
    int fifty_move;
    uint64_t hash_key;
    
    // PST Evaluation
    int material_score;
    int pst_score;
    
    // History
    std::vector<UndoMove> history;
    
    Board();
    
    void reset();
    void set_fen(const std::string& fen);
    void print();
    
    bool make_move(Move m);
    void unmake_move();
    
    // Convert algebraic to Move
    Move parse_move(const std::string& move_str);
    std::string move_to_string(Move m);
    
    // Pseudo-legal move check helpers
    bool is_square_attacked(int sq, int attacker_side);
    bool is_in_check(int side_to_check);
    
    // Opening Book
    static std::string get_book_move(const std::string& history);
    
    uint64_t generate_pos_key();
    
private:
    void update_pst_score(int piece, int sq, bool is_add);
};

// Evaluation functions
int get_piece_value(int piece);
int get_pst_value(int piece, int sq);

#endif
