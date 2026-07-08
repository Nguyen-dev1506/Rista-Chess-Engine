#include "uci.h"
#include "search.h"
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

const std::string START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

std::string current_history_str = "";

void parse_position(Board& board, const std::string& command) {
    std::istringstream iss(command);
    std::string token;
    iss >> token; // skip "position"
    
    iss >> token;
    if (token == "startpos") {
        board.set_fen(START_FEN);
        iss >> token; // skip "moves" if it exists
    } else if (token == "fen") {
        std::string fen;
        while (iss >> token && token != "moves") {
            fen += token + " ";
        }
        board.set_fen(fen);
    } else {
        board.set_fen(START_FEN);
    }
    
    current_history_str = "";
    bool first = true;
    
    // Parse moves
    while (iss >> token) {
        if (!first) current_history_str += " ";
        current_history_str += token;
        first = false;
        
        Move m = board.parse_move(token);
        if (m != 0) {
            board.make_move(m);
        }
    }
}

void parse_go(Board& board, const std::string& command) {
    int depth = 5; // Default depth
    
    std::istringstream iss(command);
    std::string token;
    while (iss >> token) {
        if (token == "depth") {
            iss >> depth;
        }
    }
    
    // 1. Check Opening Book
    std::string book_move = Board::get_book_move(current_history_str);
    if (!book_move.empty()) {
        std::cout << "bestmove " << book_move << std::endl;
        return; // ĐÃ TÌM THẤY TRONG BOOK -> IN RA VÀ NGẮT NGAY, KHÔNG CHO MINIMAX CHẠY!
    }
    
    // 2. Run Advanced Search
    Move best_move = search(board, depth);
    if (best_move == 0) {
        // Fallback if no moves
        std::cout << "bestmove 0000" << std::endl;
    } else {
        std::cout << "bestmove " << board.move_to_string(best_move) << std::endl;
    }
}

void uci_loop() {
    std::string line;
    Board board;
    board.set_fen(START_FEN);
    
    std::cout.setf(std::ios::unitbuf); // Unbuffered output
    
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string token;
        iss >> token;
        
        if (token == "uci") {
            std::cout << "id name Rista Advanced" << std::endl;
            std::cout << "id author Developer" << std::endl;
            std::cout << "uciok" << std::endl;
        } else if (token == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (token == "ucinewgame") {
            board.set_fen(START_FEN);
            current_history_str = "";
        } else if (token == "position") {
            parse_position(board, line);
        } else if (token == "go") {
            parse_go(board, line);
        } else if (token == "quit") {
            break;
        } else if (token == "print") {
            board.print();
        }
    }
}
