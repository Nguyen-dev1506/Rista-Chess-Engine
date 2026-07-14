#include <atomic>
#include <thread>
#include <vector>
#include "search.h"
#include "eval.h"
#include "movegen.h"
#include "eval.h"
#include "tt.h"
#include "book.h"
#include "magic.h"
#include <iostream>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>

namespace Search {
    std::atomic<uint64_t> nodes(0);
    std::atomic<bool> time_over(false);
    std::atomic<bool> own_book(true);
    // A/B test switches (default true = current behavior unchanged).
    std::atomic<bool> non_pv_check_ext(true);
    std::atomic<bool> improving_bonus_on(true);
    std::atomic<bool> singular_ext_on(true);
    std::chrono::time_point<std::chrono::steady_clock> start_time;
    int max_time_ms = 0; // hard limit: absolute cutoff, checked mid-search by check_time()
    // Soft limit: target used only to decide whether to start another ID
    // iteration (checked at the top of the depth loop). Starts at the
    // baseline passed into start_search, and may be widened (never beyond
    // max_time_ms) by the main thread when the best move keeps changing.
    std::atomic<int> effective_soft_ms(0);

    std::atomic<int> history[2][64][64];
    std::atomic<uint16_t> killer_moves[128][2];
    // Countermove: indexed by the [from][to] of the opponent's last move,
    // stores the move that refuted it (caused a beta cutoff right after).
    std::atomic<uint16_t> countermove[64][64];
    // 1-ply continuation history: indexed by (piece,to) of the previous move
    // then (piece,to) of the current move, updated the same way as history[].
    std::atomic<int> cont_history[12][64][12][64];
    // move_stack[ply]: the move played (by the parent node) to reach `ply`.
    // Set right after make_move() in negamax's move loop, one thread-local
    // slot per ply (same pattern as eval_stack below), used for countermove/
    // continuation history lookups. Sized with small headroom over 128
    // since it's written at ply+1. Declared here (before score_move) since
    // score_move() also reads it for move ordering.
    thread_local Move move_stack[130];

    int LMR[64][64];
    bool lmr_initialized = false;
    void init_LMR() {
        if (lmr_initialized) return;
        for (int d = 0; d < 64; d++) {
            for (int m = 0; m < 64; m++) {
                if (d >= 3 && m >= 2) {
                    LMR[d][m] = 1 + std::log(d) * std::log(m) / 1.75;
                } else {
                    LMR[d][m] = 0;
                }
            }
        }
        lmr_initialized = true;
    }

    const int INF = 31000;
    const int MATE = 30000;

    void clear_history() {
        for (int i = 0; i < 2; i++)
            for (int j = 0; j < 64; j++)
                for (int k = 0; k < 64; k++)
                    history[i][j][k] = 0;

        for (int i = 0; i < 128; i++) {
            killer_moves[i][0] = 0;
            killer_moves[i][1] = 0;
        }

        for (int i = 0; i < 64; i++)
            for (int j = 0; j < 64; j++)
                countermove[i][j] = 0;

        for (int i = 0; i < 12; i++)
            for (int j = 0; j < 64; j++)
                for (int k = 0; k < 12; k++)
                    for (int l = 0; l < 64; l++)
                        cont_history[i][j][k][l] = 0;
    }

    inline int score_to_tt(int score, int ply) {
        if (score >= MATE - 100) return score + ply;
        if (score <= -MATE + 100) return score - ply;
        return score;
    }

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

        if (m.move == killer_moves[ply][0]) return 900000;
        if (m.move == killer_moves[ply][1]) return 800000;

        int hist = history[board.side_to_move][m.from()][m.to()].load(std::memory_order_relaxed);

        if (ply > 0) {
            Move prev_move = move_stack[ply];
            if (prev_move.move != 0) {
                if (m.move == countermove[prev_move.from()][prev_move.to()].load(std::memory_order_relaxed)) {
                    return 700000;
                }
                Piece prev_piece = board.piece_on(prev_move.to());
                if (prev_piece != EMPTY_PIECE) {
                    Piece curr_piece = board.piece_on(m.from());
                    hist += cont_history[prev_piece][prev_move.to()][curr_piece][m.to()].load(std::memory_order_relaxed);
                }
            }
        }

        return std::min(hist, 699999);
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

    int quiescence(Board& board, int alpha, int beta, int ply) {
        check_time();
        if (time_over) return 0;
        if (ply >= 127) return Eval::evaluate(board);
        static thread_local int local_nodes_count = 0; local_nodes_count++; if(local_nodes_count >= 1024) { nodes.fetch_add(1024, std::memory_order_relaxed); local_nodes_count = 0; check_time(); }

        int original_alpha = alpha;
        uint16_t tt_move = 0;
        int tt_score = 0;
        int tt_eval = TT_NO_EVAL;
        if (TT.probe(board.hash_key, 0, alpha, beta, tt_score, tt_move, tt_eval, ply)) {
            return tt_score;
        }

        bool in_check = board.is_in_check(board.side_to_move);
        int stand_pat = 0;
        int best_score = -INF;
        if (!in_check) {
            stand_pat = (tt_eval != TT_NO_EVAL) ? tt_eval : Eval::evaluate(board);
            best_score = stand_pat;
            if (stand_pat >= beta) {
                TT.store(board.hash_key, 0, score_to_tt(stand_pat, ply), TT_BETA, 0, stand_pat);
                return stand_pat;
            }
            if (alpha < stand_pat) alpha = stand_pat;
        }

        MoveList list;
        MoveGen::generate_pseudo_legal(board, list, in_check ? false : true);
        sort_moves(board, list, tt_move, ply);

        int legal_moves = 0;
        uint16_t best_move = 0;
        for (int i = 0; i < list.count; i++) {
            Move m = list.moves[i];
            
            bool is_capture = (m.flags() == CAPTURE || m.flags() == EP_CAPTURE || m.flags() >= N_PROMO_CAP);
            if (!in_check && is_capture) {
                int captured_val = 100;
                Piece victim = board.piece_on(m.to());
                if (victim != EMPTY_PIECE) captured_val = Eval::PieceValueMG[piece_type(victim)];
                if (m.flags() >= N_PROMO) captured_val += Eval::PieceValueMG[QUEEN] - 100;
                
                if (stand_pat + captured_val + 200 <= alpha) {
                    continue;
                }
                if (see(board, m) < 0) {
                    continue;
                }
            }

            board.make_move(m);
            TT.prefetch(board.hash_key);
            if (board.is_in_check((board.side_to_move == WHITE) ? BLACK : WHITE)) {
                board.unmake_move(m);
                continue;
            }
            legal_moves++;
            int score = -quiescence(board, -beta, -alpha, ply + 1);
            board.unmake_move(m);

            if (score > best_score) {
                best_score = score;
                best_move = m.move;
                if (score > alpha) {
                    alpha = score;
                    if (alpha >= beta) break;
                }
            }
        }
        
        if (in_check && legal_moves == 0) {
            TT.store(board.hash_key, 0, score_to_tt(-MATE + ply, ply), TT_EXACT, 0, TT_NO_EVAL);
            return -MATE + ply;
        }

        TTFlag flag = TT_EXACT;
        if (best_score <= original_alpha) flag = TT_ALPHA;
        else if (best_score >= beta) flag = TT_BETA;

        if (in_check || flag == TT_BETA) {
            TT.store(board.hash_key, 0, score_to_tt(best_score, ply), flag, best_move, in_check ? TT_NO_EVAL : stand_pat);
        }
        
        return best_score;
    }

    thread_local int local_nodes_count = 0;
    thread_local int eval_stack[128];

    int negamax(Board& board, int depth, int ply, int alpha, int beta, bool do_null, int root_depth, uint16_t excluded_move) {
        local_nodes_count++;
        if (local_nodes_count >= 1024) {
            nodes.fetch_add(1024, std::memory_order_relaxed);
            local_nodes_count = 0;
            check_time();
        }
        
        if (time_over) return 0;
        
        if (ply > 0 && is_draw(board)) return 0;

        int original_alpha = alpha;

        bool is_pv = (beta - alpha > 1);

        uint16_t tt_move = 0;
        int tt_score = 0;
        int tt_eval = TT_NO_EVAL;
        bool tt_hit = TT.probe(board.hash_key, depth, alpha, beta, tt_score, tt_move, tt_eval, ply);
        if (tt_hit && !is_pv) {
            return tt_score;
        }

        if (depth <= 0) {
            return quiescence(board, alpha, beta, ply);
        }


        bool in_check = board.is_in_check(board.side_to_move);
        if (in_check) depth++;

        int eval = (tt_eval != TT_NO_EVAL) ? tt_eval : Eval::evaluate(board);
        eval_stack[ply] = eval;
        // "Improving": static eval got better compared to our own last move
        // (same side to move, 2 plies ago). Defaults to false (current tight
        // margins) when no ancestor data is available yet (ply < 2).
        bool improving = (ply >= 2) && (eval > eval_stack[ply - 2]);

        U64 non_pawn = board.pieces[board.side_to_move == WHITE ? W_KNIGHT : B_KNIGHT] |
                       board.pieces[board.side_to_move == WHITE ? W_BISHOP : B_BISHOP] |
                       board.pieces[board.side_to_move == WHITE ? W_ROOK : B_ROOK] |
                       board.pieces[board.side_to_move == WHITE ? W_QUEEN : B_QUEEN];
        int non_pawn_count = std::popcount(non_pawn);

        if (!in_check && depth < 3 && std::abs(beta) < MATE - 100) {
            int margin = 75 * depth;
            if (improving && improving_bonus_on.load(std::memory_order_relaxed)) margin += 70; // less aggressive pruning while eval is improving
            if (eval - margin >= beta) {
                return eval;
            }
        }

        if (depth >= 4 && tt_move == 0 && !in_check && is_pv) {
            int R = depth - 2;
            negamax(board, R, ply, alpha, beta, false, root_depth);
            tt_move = TT.probe_move(board.hash_key);
        }

        if (do_null && !is_pv && !in_check && depth >= 3 && ply > 0 && non_pawn_count > 1 && eval >= beta) {
            board.side_to_move = (board.side_to_move == WHITE) ? BLACK : WHITE;
            board.hash_key ^= Zobrist::side_key;
            if (board.en_passant != NO_SQ) board.hash_key ^= Zobrist::enpassant_keys[board.en_passant % 8];
            Square ep = board.en_passant;
            board.en_passant = NO_SQ;
            
            int R = 3 + depth / 4 + std::min(3, std::max(0, (eval - beta) / 200));
            int null_score = -negamax(board, depth - 1 - R, ply + 1, -beta, -beta + 1, false, root_depth);
            
            board.side_to_move = (board.side_to_move == WHITE) ? BLACK : WHITE;
            board.hash_key ^= Zobrist::side_key;
            board.en_passant = ep;
            if (board.en_passant != NO_SQ) board.hash_key ^= Zobrist::enpassant_keys[board.en_passant % 8];
            
            if (time_over) return 0;
            if (null_score >= beta) return beta;
        }

        int singular_extension = 0;
        if (singular_ext_on.load(std::memory_order_relaxed) && depth >= 6 && tt_move != 0 && excluded_move == 0 && !is_pv && std::abs(tt_score) < MATE - 100) {
            int s_depth, s_score;
            TTFlag s_flag;
            if (TT.probe_for_singular(board.hash_key, s_depth, s_score, s_flag, ply)) {
                if (s_depth >= depth - 3 && (s_flag == TT_BETA || s_flag == TT_EXACT)) {
                    int r_depth = depth / 2;
                    int margin = 30; // 0.3 pawns
                    int s_beta = s_score - margin;
                    
                    int s_res = negamax(board, r_depth, ply, s_beta - 1, s_beta, false, root_depth, tt_move);
                    
                    if (s_res < s_beta) {
                        singular_extension = 1; // It is singular!
                    }
                }
            }
        }

        MoveList list;
        MoveGen::generate_pseudo_legal(board, list, false);
        sort_moves(board, list, tt_move, ply);

        int legal_moves = 0;
        int quiet_moves = 0;
        int best_score = -INF;
        uint16_t best_move = 0;
        
        uint16_t quiet_searched[64];
        int num_quiet_searched = 0;
        Color stm = board.side_to_move;
        
        for (int i = 0; i < list.count; i++) {
            Move m = list.moves[i];
            if (m.move == excluded_move) continue;
            
            board.make_move(m);
            TT.prefetch(board.hash_key);
            move_stack[ply + 1] = m; // record for countermove/continuation history at the child node
            if (board.is_in_check((board.side_to_move == WHITE) ? BLACK : WHITE)) {
                board.unmake_move(m);
                continue;
            }
            legal_moves++;

            bool gives_check = board.is_in_check(board.side_to_move);
            bool is_capture = (m.flags() == CAPTURE || m.flags() == EP_CAPTURE || m.flags() >= N_PROMO_CAP);
            bool is_quiet = (m.flags() == QUIET || m.flags() == DOUBLE_PAWN || m.flags() == KING_CASTLE || m.flags() == QUEEN_CASTLE);
            
            if (is_quiet) {
                quiet_moves++;
                if (num_quiet_searched < 64) {
                    quiet_searched[num_quiet_searched++] = m.move;
                }
            }

            int extension = 0;
            if (m.move == tt_move) extension += singular_extension;
            if (gives_check) {
                // Only PV nodes get a move-level extension here. Non-PV nodes
                // already get +1 depth from the node-level "if (in_check)
                // depth++" at the top of negamax when the child is searched;
                // adding a second extension on top of that double-counts the
                // same check. In check-heavy/tactical positions (many moves
                // giving check at many non-PV nodes) that doubling multiplies
                // the tree size without a matching depth gain, collapsing
                // effective search depth (verified: depth ~9 instead of the
                // usual 15-26 at equal movetime). Keeping the PV extension
                // unconditional preserves deep forced-mate/sacrifice lines.
                if (is_pv) extension = 1;
            } else if (piece_type(board.piece_on(m.to())) == PAWN) {
                int r = m.to() / 8;
                if (r == 6 || r == 1) {
                    if (is_pv) extension = 1;
                }
            }
            if (extension > 0 && ply >= root_depth + 4) {
                extension = 0;
            }

            if (!is_pv && !in_check && !gives_check && is_quiet && depth <= 3) {
                int lmp_threshold = 3 + depth * depth;
                if (improving && improving_bonus_on.load(std::memory_order_relaxed)) lmp_threshold += 2 + depth; // allow a few more quiet moves while improving
                if (quiet_moves > lmp_threshold) {
                    board.unmake_move(m);
                    continue;
                }
            }

            if (!is_pv && !in_check && !gives_check && is_quiet && depth <= 3 && legal_moves > 1) {
                int futility_margin = 100 * depth;
                if (improving && improving_bonus_on.load(std::memory_order_relaxed)) futility_margin += 70; // less aggressive pruning while eval is improving
                if (eval + futility_margin <= alpha) {
                    board.unmake_move(m);
                    continue;
                }
            }

            int score;
            
            if (legal_moves == 1) {
                score = -negamax(board, depth - 1 + extension, ply + 1, -beta, -alpha, true, root_depth);
            } else {
                int R = 0;
                if (depth >= 3 && !in_check && !gives_check && !is_capture && is_quiet && m.move != killer_moves[ply][0]) {
                    R = LMR[std::min(63, depth)][std::min(63, legal_moves)];
                    if (!is_pv) R++;
                    
                    int hist = history[stm][m.from()][m.to()].load(std::memory_order_relaxed);
                    if (hist > 4000) R--;
                    else if (hist < -4000) R++;
                    
                    R = std::max(0, R);
                }
                
                score = -negamax(board, depth - 1 + extension - R, ply + 1, -alpha - 1, -alpha, true, root_depth);
                
                if (score > alpha) {
                    if (R > 0) {
                        score = -negamax(board, depth - 1 + extension, ply + 1, -alpha - 1, -alpha, true, root_depth);
                    }
                    if (score > alpha && score < beta) {
                        score = -negamax(board, depth - 1 + extension, ply + 1, -beta, -alpha, true, root_depth);
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
                            if (m.move != killer_moves[ply][0]) {
                                killer_moves[ply][1] = killer_moves[ply][0].load();
                                killer_moves[ply][0] = m.move;
                            }
                            int bonus = std::min(300, depth * depth);
                            
                            int v = history[stm][m.from()][m.to()].load(std::memory_order_relaxed);
                            history[stm][m.from()][m.to()].store(v + bonus - v * std::abs(bonus) / 16384, std::memory_order_relaxed);
                            
                            for (int qi = 0; qi < num_quiet_searched; qi++) {
                                Move qm(quiet_searched[qi]);
                                if (qm.move != m.move) {
                                    int qv = history[stm][qm.from()][qm.to()].load(std::memory_order_relaxed);
                                    history[stm][qm.from()][qm.to()].store(qv - bonus - qv * std::abs(bonus) / 16384, std::memory_order_relaxed);
                                }
                            }

                            // Countermove + 1-ply continuation history, same
                            // gravity-based update pattern as history[] above.
                            if (ply > 0) {
                                Move prev_move = move_stack[ply];
                                if (prev_move.move != 0) {
                                    countermove[prev_move.from()][prev_move.to()].store(m.move, std::memory_order_relaxed);

                                    Piece prev_piece = board.piece_on(prev_move.to());
                                    if (prev_piece != EMPTY_PIECE) {
                                        Square prev_to = prev_move.to();
                                        Piece curr_piece = board.piece_on(m.from());

                                        int cv = cont_history[prev_piece][prev_to][curr_piece][m.to()].load(std::memory_order_relaxed);
                                        cont_history[prev_piece][prev_to][curr_piece][m.to()].store(cv + bonus - cv * std::abs(bonus) / 16384, std::memory_order_relaxed);

                                        for (int qi = 0; qi < num_quiet_searched; qi++) {
                                            Move qm(quiet_searched[qi]);
                                            if (qm.move != m.move) {
                                                Piece qm_piece = board.piece_on(qm.from());
                                                int qcv = cont_history[prev_piece][prev_to][qm_piece][qm.to()].load(std::memory_order_relaxed);
                                                cont_history[prev_piece][prev_to][qm_piece][qm.to()].store(qcv - bonus - qcv * std::abs(bonus) / 16384, std::memory_order_relaxed);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }

        if (legal_moves == 0) {
            int score = in_check ? -MATE + ply : 0;
            TT.store(board.hash_key, depth, score_to_tt(score, ply), TT_EXACT, 0, eval);
            return score;
        }

        TTFlag flag = TT_EXACT;
        if (best_score <= original_alpha) flag = TT_ALPHA;
        else if (best_score >= beta) flag = TT_BETA;

        TT.store(board.hash_key, depth, score_to_tt(best_score, ply), flag, best_move, eval);

        return best_score;
    }

    void start_search(Board board, int depth_limit, int hard_time_ms, int soft_time_ms, int num_threads) {
        init_LMR();
        TT.new_search();
        nodes = 0;
        time_over = false;
        
        if (own_book.load()) {
            uint16_t book_move = Book::get_move(board);
            if (book_move != 0) {
                Move m(book_move);
                std::cout << "bestmove ";
                std::cout << (char)('a' + (m.from() % 8)) << (char)('1' + (m.from() / 8));
                std::cout << (char)('a' + (m.to() % 8)) << (char)('1' + (m.to() / 8));
                if (m.flags() >= N_PROMO) {
                    if (m.flags() == Q_PROMO || m.flags() == Q_PROMO_CAP) std::cout << 'q';
                    if (m.flags() == R_PROMO || m.flags() == R_PROMO_CAP) std::cout << 'r';
                    if (m.flags() == B_PROMO || m.flags() == B_PROMO_CAP) std::cout << 'b';
                    if (m.flags() == N_PROMO || m.flags() == N_PROMO_CAP) std::cout << 'n';
                }
                std::cout << std::endl;
                return;
            }
        }

        start_time = std::chrono::steady_clock::now();
        max_time_ms = hard_time_ms;
        effective_soft_ms.store(soft_time_ms, std::memory_order_relaxed);

        std::atomic<uint16_t> global_best_move(0);

        auto worker = [&](int thread_id) {
            Board thread_board = board;
            int best_score = 0;
            uint16_t prev_best_move = 0;
            int instability_count = 0;

            for (int d = 1; d <= depth_limit; d++) {
                // Soft-limit check: decide whether it's worth starting a new
                // ID iteration at all (separate from the hard limit, which is
                // still enforced mid-search by check_time()/time_over below).
                if (d > 1 && max_time_ms > 0) {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
                    if (elapsed_ms >= effective_soft_ms.load(std::memory_order_relaxed)) {
                        break;
                    }
                }

                int alpha = -INF;
                int beta = INF;
                int delta = 15;
                
                if (d >= 4) {
                    alpha = std::max(-INF, best_score - delta);
                    beta = std::min(INF, best_score + delta);
                }
                
                int score;
                while (true) {
                    int search_depth = d + (thread_id % 2); 
                    if (search_depth > depth_limit) search_depth = depth_limit;

                    score = negamax(thread_board, search_depth, 0, alpha, beta, true, search_depth);
                    if (time_over) break;
                    
                    if (score <= alpha) {
                        alpha = std::max(-INF, alpha - delta);
                        delta += delta;
                    } else if (score >= beta) {
                        beta = std::min(INF, beta + delta);
                        delta += delta;
                    } else {
                        break;
                    }
                }
                if (time_over) break;
                
                best_score = score;
                
                if (thread_id == 0) {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed_ms = std::max(1LL, (long long)std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count());
                    uint64_t total_nodes = nodes.load(std::memory_order_relaxed) + local_nodes_count;
                    uint64_t nps = (total_nodes * 1000) / elapsed_ms;
                    
                    std::cout << "info depth " << d << " ";
                    if (best_score >= MATE - 100) {
                        std::cout << "score mate " << (MATE - best_score + 1) / 2;
                    } else if (best_score <= -MATE + 100) {
                        std::cout << "score mate " << (-MATE - best_score) / 2;
                    } else {
                        std::cout << "score cp " << best_score;
                    }
                    std::cout << " nodes " << total_nodes << " nps " << nps << " time " << elapsed_ms << " pv ";
                    
                    uint16_t current_tt = TT.probe_move(thread_board.hash_key);
                    if (current_tt != 0) {
                        global_best_move.store(current_tt, std::memory_order_relaxed);

                        // Best-move instability: widen the soft limit (never past
                        // the hard limit) if the best move keeps changing across
                        // consecutive depths, so we get a bit more time to confirm it.
                        if (prev_best_move != 0 && current_tt != prev_best_move) {
                            instability_count++;
                            if (instability_count >= 2 && max_time_ms > 0) {
                                int base_soft = effective_soft_ms.load(std::memory_order_relaxed);
                                int widened = base_soft + soft_time_ms / 2;
                                effective_soft_ms.store(std::min(widened, max_time_ms), std::memory_order_relaxed);
                            }
                        } else {
                            instability_count = 0;
                        }
                        prev_best_move = current_tt;
                    }

                    Board temp = thread_board;
                    for (int i = 0; i < d; i++) {
                        uint16_t pv_move = TT.probe_move(temp.hash_key);
                        if (pv_move == 0) break;
                        
                        Move m(pv_move);
                        char pv_str[6];
                        pv_str[0] = 'a' + (m.from() % 8);
                        pv_str[1] = '1' + (m.from() / 8);
                        pv_str[2] = 'a' + (m.to() % 8);
                        pv_str[3] = '1' + (m.to() / 8);
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
                    std::cout << std::endl;
                }
            }
        };

        std::vector<std::thread> threads;
        for (int i = 1; i < num_threads; i++) {
            threads.emplace_back(worker, i);
        }
        
        worker(0);
        
        for (auto& t : threads) {
            if (t.joinable()) t.join();
        }

        uint16_t best_move = global_best_move.load(std::memory_order_relaxed);
        if (best_move == 0) best_move = TT.probe_move(board.hash_key);
        
        if (best_move == 0) {
            MoveList list;
            MoveGen::generate_pseudo_legal(board, list, false);
            int max_score = -INF;
            for(int i = 0; i < list.count; i++) {
                board.make_move(list.moves[i]);
                if (!board.is_in_check((board.side_to_move == WHITE) ? BLACK : WHITE)) {
                    int score = -quiescence(board, -INF, INF, 1);
                    if (score > max_score || best_move == 0) {
                        max_score = score;
                        best_move = list.moves[i].move;
                    }
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
        std::cout << std::endl;
    }

    uint64_t perft(Board& board, int depth) {
        if (depth == 0) return 1ULL;
        
        MoveList list;
        MoveGen::generate_pseudo_legal(board, list, false);
        
        std::atomic<uint64_t> nodes(0);
        for (int i = 0; i < list.count; i++) {
            Move m = list.moves[i];
            board.make_move(m);
            if (!board.is_in_check((board.side_to_move == WHITE) ? BLACK : WHITE)) {
                nodes += perft(board, depth - 1);
            }
            board.unmake_move(m);
        }
        return nodes;
    }
}
