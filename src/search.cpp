#include "search.h"
#include "eval.h"
#include "tt.h"
#include "zobrist.h"
#include <algorithm>
#include <iostream>
#include <cstring>

const int INF = 30000;
const int MATE = 20000;
const int MAX_DEPTH = 64;

Move killer_moves[MAX_DEPTH][2];
int history_moves[13][120];



int move_score(Board& board, Move m, int ply, Move hash_move) {
    if (m == hash_move) return 100000;
    
    int score = 0;
    int flag = move_flag(m);
    int to = move_to(m);
    int from = move_from(m);
    
    if (flag == FLAG_PROMOTION) score += 90000;
    
    if (board.pieces[to] != EMPTY || (flag == FLAG_NONE && to == board.en_passant && board.pieces[from] == W_PAWN)) {
        int victim_val = std::abs(get_piece_value(board.pieces[to]));
        if (victim_val == 0) victim_val = 100;
        int attacker_val = std::abs(get_piece_value(board.pieces[from]));
        score += 10000 + victim_val * 10 - attacker_val;
    } else {
        if (ply < MAX_DEPTH) {
            if (killer_moves[ply][0] == m) score += 9000;
            else if (killer_moves[ply][1] == m) score += 8000;
            else score += history_moves[get_piece_index(board.pieces[from])][to];
        }
    }
    return score;
}

void sort_moves(Board& board, MoveList& moves, int ply, Move hash_move) {
    int scores[256];
    for (int i = 0; i < moves.count; i++) {
        scores[i] = move_score(board, moves.moves[i], ply, hash_move);
    }
    
    // Insertion sort
    for (int i = 1; i < moves.count; i++) {
        int j = i;
        while (j > 0 && scores[j - 1] < scores[j]) {
            std::swap(scores[j - 1], scores[j]);
            std::swap(moves.moves[j - 1], moves.moves[j]);
            j--;
        }
    }
}

int qsearch(Board& board, int alpha, int beta, int ply) {
    int stand_pat = evaluate(board);
    if (stand_pat >= beta) return beta;
    if (alpha < stand_pat) alpha = stand_pat;
    
    MoveList moves;
    generate_captures(board, moves);
    sort_moves(board, moves, ply, 0);
    
    for (int i = 0; i < moves.count; i++) {
        Move m = moves.moves[i];
        if (board.make_move(m)) {
            int score = -qsearch(board, -beta, -alpha, ply + 1);
            board.unmake_move();
            
            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        }
    }
    return alpha;
}

int negamax(Board& board, int depth, int alpha, int beta, int ply, bool do_null) {
    Move hash_move = 0;
    int tt_val = probe_tt(board.hash_key, depth, alpha, beta, hash_move);
    if (tt_val != UNKNOWN_SCORE && ply > 0) return tt_val;
    
    if (depth <= 0) return qsearch(board, alpha, beta, ply);
    
    bool in_check = board.is_in_check(board.side);
    if (in_check) depth++; // Check extension
    
    // Null Move Pruning
    if (do_null && depth >= 3 && !in_check && ply > 0) {
        bool has_pieces = false;
        for (int i = 0; i < 120; i++) {
            int p = board.pieces[i];
            if (p != EMPTY && p != OFFBOARD && std::abs(p) != W_KING && std::abs(p) != W_PAWN) {
                if ((board.side == WHITE && p > 0) || (board.side == BLACK && p < 0)) {
                    has_pieces = true; break;
                }
            }
        }
        if (has_pieces) {
            int ep_sq = board.en_passant;
            if (ep_sq != SQ_NONE) board.hash_key ^= ep_keys[ep_sq];
            board.en_passant = SQ_NONE;
            
            board.side ^= 1;
            board.hash_key ^= side_key;
            
            int R = 2;
            int score = -negamax(board, depth - 1 - R, -beta, -beta + 1, ply + 1, false);
            
            board.side ^= 1;
            board.hash_key ^= side_key;
            
            board.en_passant = ep_sq;
            if (ep_sq != SQ_NONE) board.hash_key ^= ep_keys[ep_sq];
            
            if (score >= beta) return beta;
        }
    }
    
    MoveList moves;
    generate_moves(board, moves);
    sort_moves(board, moves, ply, hash_move);
    
    int legal_moves = 0;
    int best_score = -INF;
    Move best_move = 0;
    int old_alpha = alpha;
    
    for (int i = 0; i < moves.count; i++) {
        Move m = moves.moves[i];
        if (board.make_move(m)) {
            legal_moves++;
            int score;
            
            if (legal_moves == 1) {
                // Principal Variation
                score = -negamax(board, depth - 1, -beta, -alpha, ply + 1, true);
            } else {
                int reduction = 0;
                int flag = move_flag(m);
                // Late Move Reduction
                if (depth >= 3 && legal_moves > 4 && !in_check && flag != FLAG_CAPTURE && flag != FLAG_PROMOTION) {
                    reduction = 1;
                }
                
                // Zero Window Search
                score = -negamax(board, depth - 1 - reduction, -alpha - 1, -alpha, ply + 1, true);
                
                // Re-search
                if (score > alpha && score < beta) {
                    score = -negamax(board, depth - 1, -beta, -alpha, ply + 1, true);
                }
            }
            
            board.unmake_move();
            
            if (score > best_score) {
                best_score = score;
                best_move = m;
            }
            
            if (score > alpha) {
                alpha = score;
            }
            
            if (alpha >= beta) {
                int flag = move_flag(m);
                if (flag != FLAG_CAPTURE && flag != FLAG_PROMOTION && board.pieces[move_to(m)] == EMPTY) {
                    if (ply < MAX_DEPTH && killer_moves[ply][0] != m) {
                        killer_moves[ply][1] = killer_moves[ply][0];
                        killer_moves[ply][0] = m;
                    }
                    history_moves[get_piece_index(board.pieces[move_from(m)])][move_to(m)] += depth * depth;
                }
                break; // Beta Cutoff
            }
        }
    }
    
    if (legal_moves == 0) {
        if (in_check) return -MATE + ply;
        else return 0;
    }
    
    int tt_flag = HASH_EXACT;
    if (best_score <= old_alpha) tt_flag = HASH_ALPHA;
    else if (best_score >= beta) tt_flag = HASH_BETA;
    
    store_tt(board.hash_key, depth, tt_flag, best_score, best_move);
    
    return best_score;
}

Move search(Board& board, int depth_target) {
    memset(killer_moves, 0, sizeof(killer_moves));
    memset(history_moves, 0, sizeof(history_moves));
    
    Move best_overall_move = 0;
    
    for (int depth = 1; depth <= depth_target; depth++) {
        int score = negamax(board, depth, -INF, INF, 0, true);
        
        Move hash_move = 0;
        probe_tt(board.hash_key, depth, -INF, INF, hash_move);
        if (hash_move != 0) best_overall_move = hash_move;
        
        std::cout << "info depth " << depth << " score cp " << score 
                  << " pv " << board.move_to_string(best_overall_move) << std::endl;
    }
    
    return best_overall_move;
}
