#include "search.h"
#include "eval.h"
#include "movegen.h"
#include "tt.h"
#include "magic.h"
#include <iostream>
#include <algorithm>
#include <array>
#include <bit>

namespace Search {
    uint64_t nodes = 0;
    bool time_over = false;
    std::chrono::time_point<std::chrono::steady_clock> start_time;
    int max_time_ms = 0;

    std::array<std::array<std::array<int, 64>, 64>, 2> history;
    std::array<std::array<Move, 2>, 128> killer_moves;

    const int INF = 50000;
    const int MATE = 49000;

    void check_time() {
        if (max_time_ms > 0 && (nodes & 2047) == 0) {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count() >= max_time_ms) {
                time_over = true;
            }
        }
    }

    bool is_draw(const Board& board) {
        if (board.half_moves >= 100) return true;
        int end = std::max(0, board.state_ply - board.half_moves);
        for (int i = board.state_ply - 2; i >= end; i -= 2) {
            if (board.state_history[i].hash == board.hash_key) {
                return true;
            }
        }
        return false;
    }

    int see(const Board& board, Move move) {
        Square from = move.from();
        Square to = move.to();
        Piece p = board.piece_on(from);
        Piece target = board.piece_on(to);
        
        int gain[32];
        int d = 0;
        
        gain[d] = target != EMPTY_PIECE ? Eval::PieceValueMG[piece_type(target)] : 0;
        if (move.flags() >= N_PROMO) {
            Piece promo_piece = static_cast<Piece>((board.side_to_move == WHITE ? W_KNIGHT : B_KNIGHT) + (move.flags() & 3));
            gain[d] += Eval::PieceValueMG[piece_type(promo_piece)] - Eval::PieceValueMG[PAWN];
        } else if (move.flags() == EP_CAPTURE) {
            gain[d] = Eval::PieceValueMG[PAWN];
        }
        
        U64 occ = board.occ();
        occ &= ~(1ULL << from);
        
        Color stm = (board.side_to_move == WHITE) ? BLACK : WHITE;
        Piece attacker = p;
        
        while (true) {
            d++;
            Square atk_sq = NO_SQ;
            Piece atk_piece = EMPTY_PIECE;
            
            U64 side_occ = board.colors[stm] & occ;
            if (side_occ) {
                U64 p_att = Bitboards::PawnAttacks[stm == WHITE ? BLACK : WHITE][to] & board.pieces[stm == WHITE ? W_PAWN : B_PAWN] & occ;
                if (p_att) { atk_sq = static_cast<Square>(std::countr_zero(p_att)); atk_piece = stm == WHITE ? W_PAWN : B_PAWN; }
                else {
                    U64 n_att = Bitboards::KnightAttacks[to] & board.pieces[stm == WHITE ? W_KNIGHT : B_KNIGHT] & occ;
                    if (n_att) { atk_sq = static_cast<Square>(std::countr_zero(n_att)); atk_piece = stm == WHITE ? W_KNIGHT : B_KNIGHT; }
                    else {
                        U64 b_att = Magic::get_bishop_attacks(to, occ) & board.pieces[stm == WHITE ? W_BISHOP : B_BISHOP] & occ;
                        if (b_att) { atk_sq = static_cast<Square>(std::countr_zero(b_att)); atk_piece = stm == WHITE ? W_BISHOP : B_BISHOP; }
                        else {
                            U64 r_att = Magic::get_rook_attacks(to, occ) & board.pieces[stm == WHITE ? W_ROOK : B_ROOK] & occ;
                            if (r_att) { atk_sq = static_cast<Square>(std::countr_zero(r_att)); atk_piece = stm == WHITE ? W_ROOK : B_ROOK; }
                            else {
                                U64 q_att = (Magic::get_bishop_attacks(to, occ) | Magic::get_rook_attacks(to, occ)) & board.pieces[stm == WHITE ? W_QUEEN : B_QUEEN] & occ;
                                if (q_att) { atk_sq = static_cast<Square>(std::countr_zero(q_att)); atk_piece = stm == WHITE ? W_QUEEN : B_QUEEN; }
                                else {
                                    U64 k_att = Bitboards::KingAttacks[to] & board.pieces[stm == WHITE ? W_KING : B_KING] & occ;
                                    if (k_att) { atk_sq = static_cast<Square>(std::countr_zero(k_att)); atk_piece = stm == WHITE ? W_KING : B_KING; }
                                }
                            }
                        }
                    }
                }
            }
            
            if (atk_sq == NO_SQ) break;
            
            gain[d] = Eval::PieceValueMG[piece_type(attacker)] - gain[d - 1];
            if (std::max(-gain[d-1], gain[d]) < 0) break;
            
            occ &= ~(1ULL << atk_sq);
            attacker = atk_piece;
            stm = (stm == WHITE) ? BLACK : WHITE;
        }
        
        while (--d) {
            gain[d - 1] = -std::max(-gain[d - 1], gain[d]);
        }
        return gain[0];
    }

    int score_move(const Board& board, Move m, uint16_t tt_move, int ply) {
        if (m.move == tt_move) return 2000000;
        
        bool is_capture = (m.flags() == CAPTURE || m.flags() == EP_CAPTURE || m.flags() >= N_PROMO_CAP);
        
        if (is_capture) {
            Piece attacker = board.piece_on(m.from());
            Piece victim = board.piece_on(m.to());
            if (victim == EMPTY_PIECE) victim = W_PAWN;
            
            int v_val = Eval::PieceValueMG[piece_type(victim)];
            int a_val = Eval::PieceValueMG[piece_type(attacker)];
            
            int base_score = 1000000 + v_val * 10 - a_val;
            
            if (see(board, m) < 0) {
                return base_score - 2000000;
            } else {
                return base_score;
            }
        }
        
        if (m.flags() >= N_PROMO) {
             return 950000 + Eval::PieceValueMG[piece_type(static_cast<Piece>((board.side_to_move == WHITE ? W_KNIGHT : B_KNIGHT) + (m.flags() & 3)))];
        }

        if (m == killer_moves[ply][0]) return 900000;
        if (m == killer_moves[ply][1]) return 800000;
        
        return std::min(history[board.side_to_move][m.from()][m.to()], 799999);
    }

    void sort_moves(const Board& board, MoveList& list, uint16_t tt_move, int ply) {
        int scores[256];
        for (int i = 0; i < list.count; i++) {
            scores[i] = score_move(board, list.moves[i], tt_move, ply);
        }
        for (int i = 1; i < list.count; i++) {
            int j = i;
            while (j > 0 && scores[j - 1] < scores[j]) {
                std::swap(scores[j - 1], scores[j]);
                std::swap(list.moves[j - 1], list.moves[j]);
                j--;
            }
        }
    }

    int quiescence(Board& board, int alpha, int beta) {
        check_time();
        if (time_over) return 0;
        nodes++;

        int stand_pat = Eval::evaluate(board);
        if (stand_pat >= beta) return beta;
        if (alpha < stand_pat) alpha = stand_pat;

        MoveList list;
        MoveGen::generate_pseudo_legal(board, list, true);
        sort_moves(board, list, 0, 0);

        for (int i = 0; i < list.count; i++) {
            Move m = list.moves[i];
            
            if (see(board, m) < 0) continue;

            board.make_move(m);
            if (board.is_in_check((board.side_to_move == WHITE) ? BLACK : WHITE)) { 
                board.unmake_move(m);
                continue;
            }
            int score = -quiescence(board, -beta, -alpha);
            board.unmake_move(m);

            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        }
        return alpha;
    }

    int negamax(Board& board, int depth, int ply, int alpha, int beta, bool do_null) {
        check_time();
        if (time_over) return 0;
        
        if (ply > 0 && is_draw(board)) return 0;

        int original_alpha = alpha;
        
        uint16_t tt_move = 0;
        int tt_score = 0;
        if (TT.probe(board.hash_key, depth, alpha, beta, tt_score, tt_move)) {
            return tt_score;
        }

        if (depth <= 0) {
            return quiescence(board, alpha, beta);
        }

        nodes++;
        bool in_check = board.is_in_check(board.side_to_move);
        if (in_check) depth++;

        int eval = Eval::evaluate(board);

        U64 non_pawn = board.pieces[board.side_to_move == WHITE ? W_KNIGHT : B_KNIGHT] |
                       board.pieces[board.side_to_move == WHITE ? W_BISHOP : B_BISHOP] |
                       board.pieces[board.side_to_move == WHITE ? W_ROOK : B_ROOK] |
                       board.pieces[board.side_to_move == WHITE ? W_QUEEN : B_QUEEN];
        bool has_non_pawn = (non_pawn != 0);

        if (!in_check && depth < 3 && std::abs(beta) < MATE - 100) {
            int margin = 120 * depth; 
            if (eval - margin >= beta) {
                return eval - margin;
            }
        }

        if (depth >= 4 && tt_move == 0 && !in_check) {
            int R = depth - 2;
            negamax(board, R, ply, alpha, beta, false);
            tt_move = TT.probe_move(board.hash_key);
        }

        if (do_null && !in_check && depth >= 3 && ply > 0 && has_non_pawn && eval >= beta) {
            board.side_to_move = (board.side_to_move == WHITE) ? BLACK : WHITE;
            board.hash_key ^= Zobrist::side_key;
            if (board.en_passant != NO_SQ) board.hash_key ^= Zobrist::enpassant_keys[board.en_passant % 8];
            Square ep = board.en_passant;
            board.en_passant = NO_SQ;
            
            int R = 3 + depth / 4 + std::min(3, std::max(0, (eval - beta) / 200));
            int null_score = -negamax(board, depth - 1 - R, ply + 1, -beta, -beta + 1, false);
            
            board.side_to_move = (board.side_to_move == WHITE) ? BLACK : WHITE;
            board.hash_key ^= Zobrist::side_key;
            board.en_passant = ep;
            if (board.en_passant != NO_SQ) board.hash_key ^= Zobrist::enpassant_keys[board.en_passant % 8];
            
            if (time_over) return 0;
            if (null_score >= beta) return beta;
        }

        MoveList list;
        MoveGen::generate_pseudo_legal(board, list, false);
        sort_moves(board, list, tt_move, ply);

        int legal_moves = 0;
        int best_score = -INF;
        uint16_t best_move = 0;
        
        for (int i = 0; i < list.count; i++) {
            Move m = list.moves[i];
            board.make_move(m);
            if (board.is_in_check((board.side_to_move == WHITE) ? BLACK : WHITE)) {
                board.unmake_move(m);
                continue;
            }
            legal_moves++;

            bool gives_check = board.is_in_check(board.side_to_move);
            bool is_capture = (m.flags() == CAPTURE || m.flags() == EP_CAPTURE || m.flags() >= N_PROMO_CAP);
            bool is_quiet = (m.flags() == QUIET);

            int extension = 0;
            if (piece_type(board.piece_on(m.to())) == PAWN) {
                int r = m.to() / 8;
                if (r == 6 || r == 1) {
                    extension = 1;
                }
            }

            int score;
            
            if (legal_moves == 1) {
                score = -negamax(board, depth - 1 + extension, ply + 1, -beta, -alpha, true);
            } else {
                int R = 0;
                if (depth >= 3 && !in_check && !gives_check && !is_capture && is_quiet && m.move != killer_moves[ply][0].move) {
                    R = 1 + (depth / 3) + (legal_moves / 4);
                }
                
                score = -negamax(board, depth - 1 + extension - R, ply + 1, -alpha - 1, -alpha, true);
                
                if (score > alpha) {
                    if (R > 0) {
                        score = -negamax(board, depth - 1 + extension, ply + 1, -alpha - 1, -alpha, true);
                    }
                    if (score > alpha && score < beta) {
                        score = -negamax(board, depth - 1 + extension, ply + 1, -beta, -alpha, true);
                    }
                }
            }

            board.unmake_move(m);

            if (time_over) return 0;

            if (score > best_score) {
                best_score = score;
                best_move = m.move;
                if (score > alpha) {
                    alpha = score;
                    if (alpha >= beta) {
                        if (is_quiet) {
                            if (m.move != killer_moves[ply][0].move) {
                                killer_moves[ply][1] = killer_moves[ply][0];
                                killer_moves[ply][0] = m;
                            }
                            int bonus = depth * depth;
                            history[board.side_to_move][m.from()][m.to()] += bonus;
                            if (history[board.side_to_move][m.from()][m.to()] > 1000000) {
                                for(int c=0; c<2; c++)
                                    for(int f=0; f<64; f++)
                                        for(int t=0; t<64; t++)
                                            history[c][f][t] /= 2;
                            }
                        }
                        break;
                    }
                }
            }
        }

        if (legal_moves == 0) {
            if (in_check) return -MATE + ply;
            else return 0;
        }

        TTFlag flag = TT_EXACT;
        if (best_score <= original_alpha) flag = TT_ALPHA;
        else if (best_score >= beta) flag = TT_BETA;
        
        TT.store(board.hash_key, depth, best_score, flag, best_move);

        return best_score;
    }

    void start_search(Board& board, int depth_limit, int time_limit_ms) {
        TT.new_search();
        nodes = 0;
        time_over = false;
        start_time = std::chrono::steady_clock::now();
        max_time_ms = time_limit_ms;

        for (int i = 0; i < 2; i++)
            for (int j = 0; j < 64; j++)
                for (int k = 0; k < 64; k++)
                    history[i][j][k] = 0;

        for (int i = 0; i < 128; i++) {
            killer_moves[i][0] = Move();
            killer_moves[i][1] = Move();
        }

        int best_score = 0;
        uint16_t best_move = 0;

        for (int d = 1; d <= depth_limit; d++) {
            int alpha = -INF;
            int beta = INF;
            int delta = 25;
            
            if (d >= 4) {
                alpha = std::max(-INF, best_score - delta);
                beta = std::min(INF, best_score + delta);
            }
            
            int score;
            while (true) {
                score = negamax(board, d, 0, alpha, beta, true);
                if (time_over) break;
                
                if (score <= alpha) {
                    alpha = std::max(-INF, score - delta);
                    delta *= 2;
                } else if (score >= beta) {
                    beta = std::min(INF, score + delta);
                    delta *= 2;
                } else {
                    break;
                }
            }
            if (time_over) break;
            
            best_score = score;
            uint16_t tt_move = TT.probe_move(board.hash_key);
            if (tt_move != 0) {
                best_move = tt_move;
            }
            
            std::cout << "info depth " << d << " score cp " << best_score 
                      << " nodes " << nodes << " pv ";
            
            Board temp = board;
            for (int p = 0; p < d; p++) {
                uint16_t tm = TT.probe_move(temp.hash_key);
                if (tm == 0) break;
                Move m(tm);
                Square from = m.from();
                Square to = m.to();
                char pv_str[6];
                pv_str[0] = 'a' + (from % 8);
                pv_str[1] = '1' + (from / 8);
                pv_str[2] = 'a' + (to % 8);
                pv_str[3] = '1' + (to / 8);
                pv_str[4] = '\0';
                if (m.flags() >= N_PROMO) {
                    if (m.flags() == Q_PROMO || m.flags() == Q_PROMO_CAP) pv_str[4] = 'q';
                    if (m.flags() == R_PROMO || m.flags() == R_PROMO_CAP) pv_str[4] = 'r';
                    if (m.flags() == B_PROMO || m.flags() == B_PROMO_CAP) pv_str[4] = 'b';
                    if (m.flags() == N_PROMO || m.flags() == N_PROMO_CAP) pv_str[4] = 'n';
                    pv_str[5] = '\0';
                }
                std::cout << pv_str << " ";
                temp.make_move(m);
            }
            std::cout << "\n";
        }
        
        if (best_move == 0) {
            MoveList list;
            MoveGen::generate_pseudo_legal(board, list, false);
            for(int i = 0; i < list.count; i++) {
                board.make_move(list.moves[i]);
                if (!board.is_in_check((board.side_to_move == WHITE) ? BLACK : WHITE)) {
                    best_move = list.moves[i].move;
                    board.unmake_move(list.moves[i]);
                    break;
                }
                board.unmake_move(list.moves[i]);
            }
        }

        Move m(best_move);
        std::cout << "bestmove ";
        std::cout << (char)('a' + (m.from() % 8)) << (char)('1' + (m.from() / 8));
        std::cout << (char)('a' + (m.to() % 8)) << (char)('1' + (m.to() / 8));
        if (m.flags() >= N_PROMO) {
            if (m.flags() == Q_PROMO || m.flags() == Q_PROMO_CAP) std::cout << 'q';
            if (m.flags() == R_PROMO || m.flags() == R_PROMO_CAP) std::cout << 'r';
            if (m.flags() == B_PROMO || m.flags() == B_PROMO_CAP) std::cout << 'b';
            if (m.flags() == N_PROMO || m.flags() == N_PROMO_CAP) std::cout << 'n';
        }
        std::cout << "\n";
    }
}
