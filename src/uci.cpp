#include "uci.h"
#include "board.h"
#include "search.h"
#include "movegen.h"
#include "tt.h"
#include <iostream>
#include <string>
#include <sstream>

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

    void loop() {
        std::string line;
        Board board;
        
        while (std::getline(std::cin, line)) {
            std::istringstream iss(line);
            std::string token;
            iss >> token;
            
            if (token == "uci") {
                std::cout << "id name Rista (Bitboard C++20)\n";
                std::cout << "id author You\n";
                std::cout << "option name Hash type spin default 16 min 1 max 1024\n";
                std::cout << "uciok\n";
            } else if (token == "isready") {
                std::cout << "readyok\n";
            } else if (token == "setoption") {
                std::string name, value;
                iss >> token; // name
                iss >> name;
                iss >> token; // value
                iss >> value;
                if (name == "Hash") {
                    TT.resize(std::stoi(value));
                }
            } else if (token == "ucinewgame") {
                TT.clear();
                Search::clear_history();
            } else if (token == "position") {
                std::string pos_type;
                iss >> pos_type;
                if (pos_type == "startpos") {
                    board.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                    iss >> token; // "moves"
                } else if (pos_type == "fen") {
                    std::string fen = "";
                    for (int i = 0; i < 6; i++) {
                        std::string temp;
                        iss >> temp;
                        fen += temp + " ";
                    }
                    board.set_fen(fen);
                    iss >> token; // "moves"
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
                while (iss >> token) {
                    if (token == "depth") iss >> depth;
                    if (token == "wtime") iss >> wtime;
                    if (token == "btime") iss >> btime;
                    if (token == "winc") iss >> winc;
                    if (token == "binc") iss >> binc;
                    if (token == "movestogo") iss >> movestogo;
                    if (token == "movetime") { iss >> time; has_movetime = true; }
                }
                if (!has_movetime) {
                    int my_time = (board.side_to_move == WHITE) ? wtime : btime;
                    int my_inc  = (board.side_to_move == WHITE) ? winc  : binc;
                    if (my_time > 0) {
                        int moves_left = (movestogo > 0) ? movestogo : 30;
                        time = my_time / moves_left + (my_inc * 3 / 4);
                        time = std::min(time, my_time - 50); // chừa buffer an toàn tránh timeout
                        if (time < 50) time = 50;
                    }
                }
                Search::start_search(board, depth, time);
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
                break;
            }
        }
    }
}
