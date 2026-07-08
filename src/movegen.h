#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include <vector>

void generate_moves(Board& board, std::vector<Move>& moves);
void generate_captures(Board& board, std::vector<Move>& moves);

#endif
