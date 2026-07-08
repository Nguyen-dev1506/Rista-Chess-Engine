#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include <vector>

struct MoveList {
    Move moves[256];
    int count = 0;
    
    inline void add(Move m) {
        moves[count++] = m;
    }
};

void generate_moves(Board& board, MoveList& move_list);
void generate_captures(Board& board, MoveList& move_list);

#endif
