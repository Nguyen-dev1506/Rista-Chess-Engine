#include "eval.h"
#include <cmath>

// Piece values for MG and EG: EMPTY, P, N, B, R, Q, K
const int MG_WEIGHT[7] = {0, 82, 337, 365, 477, 1025, 0};
const int EG_WEIGHT[7] = {0, 94, 281, 297, 512,  936, 0};

// Game phase values
const int PHASE_WEIGHT[7] = {0, 0, 1, 1, 2, 4, 0};
const int TOTAL_PHASE = 16 * 0 + 4 * 1 + 4 * 1 + 4 * 2 + 2 * 4; // 24

// Simplistic PST tables, indexed by sq64 (0..63) where 0 is A1
const int MG_PAWN[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
     50, 50, 50, 50, 50, 50, 50, 50,
     10, 10, 20, 30, 30, 20, 10, 10,
      5,  5, 10, 25, 25, 10,  5,  5,
      0,  0,  0, 20, 20,  0,  0,  0,
      5, -5,-10,  0,  0,-10, -5,  5,
      5, 10, 10,-20,-20, 10, 10,  5,
      0,  0,  0,  0,  0,  0,  0,  0
};

const int EG_PAWN[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
     80, 80, 80, 80, 80, 80, 80, 80,
     50, 50, 50, 50, 50, 50, 50, 50,
     30, 30, 30, 30, 30, 30, 30, 30,
     20, 20, 20, 20, 20, 20, 20, 20,
     10, 10, 10, 10, 10, 10, 10, 10,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0
};

const int MG_KNIGHT[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

const int EG_KNIGHT[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

const int MG_BISHOP[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

const int EG_BISHOP[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

const int MG_ROOK[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
      5, 10, 10, 10, 10, 10, 10,  5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
      0,  0,  0,  5,  5,  0,  0,  0
};

const int EG_ROOK[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
      5, 10, 10, 10, 10, 10, 10,  5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
      0,  0,  0,  5,  5,  0,  0,  0
};

const int MG_QUEEN[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

const int EG_QUEEN[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

const int MG_KING[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

const int EG_KING[64] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};

int mg_table[7][64];
int eg_table[7][64];

void init_eval() {
    for (int sq = 0; sq < 64; ++sq) {
        // We read visual sq so 0 is A8, 63 is H1? No, 0 is A1 for normal engines but my PST visual layout is A8 to H1.
        // Let's assume the array index corresponds directly to visual_sq like in board.cpp:
        // int visual_sq = (7 - (sq64 / 8)) * 8 + (sq64 % 8);
        // I will copy the values directly.
        mg_table[1][sq] = MG_PAWN[sq] + MG_WEIGHT[1];
        mg_table[2][sq] = MG_KNIGHT[sq] + MG_WEIGHT[2];
        mg_table[3][sq] = MG_BISHOP[sq] + MG_WEIGHT[3];
        mg_table[4][sq] = MG_ROOK[sq] + MG_WEIGHT[4];
        mg_table[5][sq] = MG_QUEEN[sq] + MG_WEIGHT[5];
        mg_table[6][sq] = MG_KING[sq] + MG_WEIGHT[6];
        
        eg_table[1][sq] = EG_PAWN[sq] + EG_WEIGHT[1];
        eg_table[2][sq] = EG_KNIGHT[sq] + EG_WEIGHT[2];
        eg_table[3][sq] = EG_BISHOP[sq] + EG_WEIGHT[3];
        eg_table[4][sq] = EG_ROOK[sq] + EG_WEIGHT[4];
        eg_table[5][sq] = EG_QUEEN[sq] + EG_WEIGHT[5];
        eg_table[6][sq] = EG_KING[sq] + EG_WEIGHT[6];
    }
}

void update_eval(Board& board, int piece, int sq, bool is_add) {
    if (piece == EMPTY || piece == OFFBOARD) return;
    
    int side = (piece > 0) ? WHITE : BLACK;
    int p_type = std::abs(piece);
    
    int sq64 = sq120_to_sq64(sq);
    if (side == BLACK) {
        sq64 = sq64 ^ 56; // mirror
    }
    
    int visual_sq = (7 - (sq64 / 8)) * 8 + (sq64 % 8);
    
    int mg = mg_table[p_type][visual_sq];
    int eg = eg_table[p_type][visual_sq];
    int phase = PHASE_WEIGHT[p_type];
    
    if (is_add) {
        board.mg_score[side] += mg;
        board.eg_score[side] += eg;
        board.game_phase += phase;
    } else {
        board.mg_score[side] -= mg;
        board.eg_score[side] -= eg;
        board.game_phase -= phase;
    }
}

int evaluate(Board& board) {
    int mg_score = board.mg_score[WHITE] - board.mg_score[BLACK];
    int eg_score = board.eg_score[WHITE] - board.eg_score[BLACK];
    
    int phase = board.game_phase;
    if (phase > 24) phase = 24;
    
    int score = (mg_score * phase + eg_score * (24 - phase)) / 24;
    
    // Quick pawn structure / mobility could be added here, but for now tapered eval is main.
    // Adding doubled pawn penalty
    int pawn_files[2][8] = {{0}};
    for (int sq = 0; sq < 120; sq++) {
        if (board.pieces[sq] == W_PAWN) pawn_files[WHITE][sq120_to_sq64(sq) % 8]++;
        else if (board.pieces[sq] == B_PAWN) pawn_files[BLACK][sq120_to_sq64(sq) % 8]++;
    }
    
    int doubled_penalty = 0;
    for (int i = 0; i < 8; i++) {
        if (pawn_files[WHITE][i] > 1) doubled_penalty -= 10 * (pawn_files[WHITE][i] - 1);
        if (pawn_files[BLACK][i] > 1) doubled_penalty += 10 * (pawn_files[BLACK][i] - 1);
    }
    
    score += doubled_penalty;
    
    return (board.side == WHITE) ? score : -score;
}
