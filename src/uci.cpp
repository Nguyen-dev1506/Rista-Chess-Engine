#include "uci.h"
#include "board.h"
#include "search.h"
#include "movegen.h"
#include "tt.h"
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <filesystem>
#include "book.h"
#include "eval.h"
extern "C" {
#include "fathom/tbprobe.h"
}

namespace UCI {
    Move parse_move(Board& board, const std::string& move_str) {
        MoveList list;
        MoveGen::generate_pseudo_legal(board, list, false);
        for (int i = 0; i < list.count; i++) {
            Move m = list.moves[i];
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
            if (move_str == std::string(pv_str)) {
                return m;
            }
        }
        return Move();
    }

    void loop(const char* exe_path) {
        Search::init_LMR();
        tb_init("syzygy");
        std::cout << "info string Syzygy tablebases default loaded, largest: " << TB_LARGEST << std::endl;
        std::filesystem::path exe_dir = std::filesystem::absolute(exe_path).parent_path();
        std::filesystem::path book_path = exe_dir / "books" / "book_small.bin";
        Book::init(book_path.string());
        if (!Book::is_loaded()) {
            std::cout << "info string Failed to load opening book, path: " << book_path.string() << "\n";
        }
        std::string line;
        Board board;
        int num_threads = 1;
        std::thread search_thread;
        
        while (std::getline(std::cin, line)) {
            std::istringstream iss(line);
            std::string token;
            iss >> token;
            
            if (token == "uci") {
                std::cout << "id name Rista (Bitboard C++20)" << std::endl;
                std::cout << "id author You" << std::endl;
                std::cout << "option name Hash type spin default 16 min 1 max 1024\n";
                std::cout << "option name Threads type spin default 1 min 1 max 128\n";
                std::cout << "option name OwnBook type check default true" << std::endl;
                std::cout << "option name NonPVCheckExt type check default true" << std::endl;
                std::cout << "option name ImprovingBonus type check default true" << std::endl;
                std::cout << "option name SingularExt type check default true" << std::endl;
                std::cout << "option name SyzygyPath type string default <empty>" << std::endl;
                std::cout << "uciok" << std::endl;
            } else if (token == "isready") {
                std::cout << "readyok" << std::endl;
            } else if (token == "setoption") {
                std::string name, value;
                iss >> token; // name
                iss >> name;
                iss >> token; // value
                iss >> std::ws;
                std::getline(iss, value);
                
                if (name == "Hash") {
                    try {
                        int hash_mb = std::stoi(value);
                        if (search_thread.joinable()) {
                            Search::time_over = true;
                            search_thread.join();
                        }
                        TT.resize(hash_mb);
                    } catch (const std::exception&) {
                        // invalid value for Hash, ignore
                    }
                } else if (name == "Threads") {
                    try {
                        int threads_val = std::stoi(value);
                        if (search_thread.joinable()) {
                            Search::time_over = true;
                            search_thread.join();
                        }
                        num_threads = threads_val;
                    } catch (const std::exception&) {
                        // invalid value for Threads, ignore
                    }
                } else if (name == "OwnBook") {
                    Search::own_book = (value == "true");
                } else if (name == "NonPVCheckExt") {
                    Search::non_pv_check_ext = (value == "true");
                } else if (name == "ImprovingBonus") {
                    Search::improving_bonus_on = (value == "true");
                } else if (name == "SingularExt") {
                    Search::singular_ext_on = (value == "true");
                } else if (name == "SyzygyPath") {
                    tb_init(value.c_str());
                    if (TB_LARGEST > 0) {
                        std::cout << "info string Syzygy tablebases loaded, largest: " << TB_LARGEST << "\n";
                    } else {
                        std::cout << "info string Syzygy tablebases not found or invalid\n";
                    }
                }
            } else if (token == "ucinewgame") {
                if (search_thread.joinable()) {
                    Search::time_over = true;
                    search_thread.join();
                }
                TT.clear();
                Search::clear_history();
            } else if (token == "position") {
                if (search_thread.joinable()) {
                    Search::time_over = true;
                    search_thread.join();
                }
                std::string pos_type;
                iss >> pos_type;
                if (pos_type == "startpos") {
                    board.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                    iss >> token; // "moves"
                } else if (pos_type == "fen") {
                    std::string fen = "";
                    bool hit_moves = false;
                    for (int i = 0; i < 6; i++) {
                        std::string temp;
                        if (!(iss >> temp)) break;
                        if (temp == "moves") {
                            hit_moves = true;
                            break;
                        }
                        fen += temp + " ";
                    }
                    board.set_fen(fen);
                    if (!hit_moves) iss >> token; // "moves"
                }
                
                while (iss >> token) {
                    Move m = parse_move(board, token);
                    if (m.is_valid()) {
                        board.make_move(m);
                    }
                }
            } else if (token == "go") {
                int depth = 64;
                int time = 0;
                int wtime = 0, btime = 0, winc = 0, binc = 0, movestogo = 0;
                bool has_movetime = false;
                long long nodes_limit = 0;
                int mate_moves = 0;
                while (iss >> token) {
                    if (token == "depth") iss >> depth;
                    if (token == "wtime") iss >> wtime;
                    if (token == "btime") iss >> btime;
                    if (token == "winc") iss >> winc;
                    if (token == "binc") iss >> binc;
                    if (token == "movestogo") iss >> movestogo;
                    if (token == "movetime") { iss >> time; has_movetime = true; }
                    if (token == "infinite") { depth = 64; }
                    if (token == "ponder") { /* not implemented; token consumed so later tokens don't desync */ }
                    if (token == "nodes") iss >> nodes_limit;
                    if (token == "mate") iss >> mate_moves;
                }
                if (!has_movetime) {
                    int my_time = (board.side_to_move == WHITE) ? wtime : btime;
                    int my_inc  = (board.side_to_move == WHITE) ? winc  : binc;
                    if (my_time > 0) {
                        int moves_left = (movestogo > 0) ? movestogo : 30;
                        time = my_time / moves_left + (my_inc * 3 / 4);
                        time = std::min(time, my_time - 50); // chừa buffer an toàn tránh timeout
                        if (my_time - 50 >= 50 && time < 50) time = 50;
                    }
                }
                // Soft limit: only used to decide whether to start another ID
                // iteration, never to abort mid-search. "time" above remains the
                // hard limit exactly as computed/fixed before -- untouched here.
                int soft_time = time;
                if (!has_movetime && time > 0) {
                    soft_time = time * 6 / 10; // ~60% of the hard limit
                }
                if (search_thread.joinable()) {
                    Search::time_over = true;
                    search_thread.join();
                }
                Search::time_over = false;
                search_thread = std::thread(Search::start_search, board, depth, time, soft_time, num_threads);
            } else if (token == "stop") {
                Search::time_over = true;
                if (search_thread.joinable()) {
                    search_thread.join();
                }
            } else if (token == "perft") {
                int depth = 5;
                if (iss >> token) depth = std::stoi(token);
                auto start = std::chrono::steady_clock::now();
                uint64_t nodes = Search::perft(board, depth);
                auto end = std::chrono::steady_clock::now();
                std::chrono::duration<double> elapsed = end - start;
                std::cout << "Depth " << depth << ": " << nodes << " nodes, "
                          << elapsed.count() << " seconds\n";
            } else if (token == "d") {
                board.print();
            } else if (token == "quit") {
                Search::time_over = true;
                if (search_thread.joinable()) {
                    search_thread.join();
                }
                break;
            }
        }
    }
}
