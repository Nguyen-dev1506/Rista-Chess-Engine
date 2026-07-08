#ifndef EVAL_H
#define EVAL_H

#include "board.h"

extern const int MG_WEIGHT[7];
extern const int EG_WEIGHT[7];

void init_eval();
int evaluate(Board& board);
void update_eval(Board& board, int piece, int sq, bool is_add);

#endif
