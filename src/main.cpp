#include "bitboard.h"
#include "magic.h"
#include "zobrist.h"
#include "uci.h"
#include <iostream>

int main() {
    std::cout << "Initializing Rista Engine (Bitboard)..." << std::endl;
    Bitboards::init();
    Magic::init();
    Zobrist::init();
    std::cout << "Initialization Complete." << std::endl;
    
    UCI::loop();
    
    return 0;
}
