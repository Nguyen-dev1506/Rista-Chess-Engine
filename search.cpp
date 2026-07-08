#include "search.h"
#include <algorithm>
#include <iostream>
#include <cstring>

const int INF = 30000;
const int MATE = 20000;
const int MAX_DEPTH = 64;

Move killer_moves[MAX_DEPTH][2];

int evaluate(Board& board) {
    int score = board.material_score + board.pst_score;
    // Điểm đứng từ góc nhìn của bên chuẩn bị đi
    return (board.side == WHITE) ? score : -score;
}

// Chấm điểm nước đi (Move Ordering)
int move_score(Board& board, Move m, int ply) {
    int score = 0;
    int flag = move_flag(m);
    int to = move_to(m);
    int from = move_from(m);
    
    // 1. Promotions (Cực kỳ ưu tiên)
    if (flag == FLAG_PROMOTION) {
        score += 90000; // Ưu tiên phong cấp Hậu
    }
    
    // 2. Captures (MVV-LVA: Most Valuable Victim - Least Valuable Attacker)
    if (board.pieces[to] != EMPTY || (flag == FLAG_NONE && to == board.en_passant && board.pieces[from] == W_PAWN)) { // Bắt quân hoặc En Passant
        int victim_val = std::abs(get_piece_value(board.pieces[to]));
        if (victim_val == 0) victim_val = 100; // En Passant victim = Pawn
        int attacker_val = std::abs(get_piece_value(board.pieces[from]));
        
        // MVV-LVA: Điểm cao nhất khi chốt ăn Hậu, điểm thấp khi Hậu ăn chốt
        score += 10000 + victim_val * 10 - attacker_val;
    } 
    // 3. Killer Moves (Nước đi không ăn quân từng gây ra Beta-Cutoff)
    else {
        if (ply < MAX_DEPTH) {
            if (killer_moves[ply][0] == m) {
                score += 9000;
            } else if (killer_moves[ply][1] == m) {
                score += 8000;
            }
        }
    }
    
    return score;
}

void sort_moves(Board& board, std::vector<Move>& moves, int ply) {
    std::vector<std::pair<int, Move>> scored_moves;
    scored_moves.reserve(moves.size());
    for (Move m : moves) {
        scored_moves.push_back({move_score(board, m, ply), m});
    }
    
    // Sort giảm dần
    std::sort(scored_moves.begin(), scored_moves.end(), [](const std::pair<int, Move>& a, const std::pair<int, Move>& b) {
        return a.first > b.first;
    });
    
    for (size_t i = 0; i < moves.size(); i++) {
        moves[i] = scored_moves[i].second;
    }
}

// Quiescence Search (Giải quyết Horizon Effect)
int qsearch(Board& board, int alpha, int beta, int ply) {
    int stand_pat = evaluate(board);
    
    if (stand_pat >= beta) {
        return beta;
    }
    if (alpha < stand_pat) {
        alpha = stand_pat;
    }
    
    std::vector<Move> moves;
    generate_captures(board, moves);
    sort_moves(board, moves, ply);
    
    for (Move m : moves) {
        if (board.make_move(m)) {
            int score = -qsearch(board, -beta, -alpha, ply + 1);
            board.unmake_move();
            
            if (score >= beta) {
                return beta;
            }
            if (score > alpha) {
                alpha = score;
            }
        }
    }
    
    return alpha;
}

int negamax(Board& board, int depth, int alpha, int beta, int ply) {
    if (depth <= 0) {
        return qsearch(board, alpha, beta, ply);
    }
    
    std::vector<Move> moves;
    generate_moves(board, moves);
    sort_moves(board, moves, ply);
    
    int legal_moves = 0;
    int best_score = -INF;
    
    for (Move m : moves) {
        if (board.make_move(m)) {
            legal_moves++;
            
            int score = -negamax(board, depth - 1, -beta, -alpha, ply + 1);
            board.unmake_move();
            
            if (score > best_score) {
                best_score = score;
            }
            
            if (score > alpha) {
                alpha = score;
            }
            
            if (alpha >= beta) {
                // Beta Cutoff - Cập nhật Killer Moves nếu đây không phải nước ăn quân
                int flag = move_flag(m);
                if (flag != FLAG_CAPTURE && flag != FLAG_PROMOTION && board.pieces[move_to(m)] == EMPTY) {
                    if (ply < MAX_DEPTH && killer_moves[ply][0] != m) {
                        killer_moves[ply][1] = killer_moves[ply][0];
                        killer_moves[ply][0] = m;
                    }
                }
                break; // Alpha-Beta pruning
            }
        }
    }
    
    if (legal_moves == 0) {
        // Kiểm tra xem Vua có đang bị chiếu không (Checkmate) hay hòa (Stalemate)
        int king_sq = SQ_NONE;
        int king_piece = (board.side == WHITE) ? W_KING : B_KING;
        for (int i = 0; i < 120; i++) {
            if (board.pieces[i] == king_piece) {
                king_sq = i;
                break;
            }
        }
        if (board.is_square_attacked(king_sq, board.side ^ 1)) {
            return -MATE + ply; // Checkmate (ply ngắn hơn sẽ được ưu tiên)
        } else {
            return 0; // Stalemate
        }
    }
    
    return best_score;
}

Move search(Board& board, int depth) {
    memset(killer_moves, 0, sizeof(killer_moves));
    
    std::vector<Move> moves;
    generate_moves(board, moves);
    sort_moves(board, moves, 0);
    
    Move best_move = 0;
    int alpha = -INF;
    int beta = INF;
    int best_score = -INF;
    
    for (Move m : moves) {
        if (board.make_move(m)) {
            int score = -negamax(board, depth - 1, -beta, -alpha, 1);
            board.unmake_move();
            
            if (depth == 6) {
                std::cout << "info string Move " << board.move_to_string(m) 
                          << " score " << score << std::endl;
            }
            if (score > best_score || best_move == 0) {
                best_score = score;
                best_move = m;
            }
            if (best_score > alpha) {
                alpha = best_score;
            }
        }
    }
    
    return best_move;
}
