#ifndef EVAL_H
#define EVAL_H

#include "board.h"

namespace Eval {
    extern const int PieceValueMG[6];
    extern const int PieceValueEG[6];
    
    int evaluate(const Board& board);
}

#endif
