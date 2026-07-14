#include "bitboard.h"
#include "magic.h"
#include "zobrist.h"
#include "tt.h"
#include "book.h"
#include "uci.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "Initializing Rista Engine (Bitboard)..." << std::endl;
    Bitboards::init();
    Magic::init();
    Zobrist::init();

    std::cout << "Initialization Complete." << std::endl;

    UCI::loop(argv[0]);

    return 0;
}
