#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include "movegen.h"

// Returns the best move found within the given depth
Move search(Board& board, int depth);

// Evaluates the current board state
int evaluate(Board& board);

#endif
