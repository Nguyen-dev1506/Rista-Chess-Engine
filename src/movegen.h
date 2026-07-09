#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include <vector>

struct MoveList {
    Move moves[256];
    int count = 0;
    
    void add(Move m) {
        moves[count++] = m;
    }
};

namespace MoveGen {
    void generate_pseudo_legal(const Board& board, MoveList& list, bool captures_only = false);
    
    // Legal move checking can be done during search by making pseudo-legal move and checking if king is in check
    // Alternatively, a strict legal move generator. We'll use the make/in_check/unmake approach for simplicity and speed in Negamax.
}

#endif
