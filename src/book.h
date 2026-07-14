#ifndef BOOK_H
#define BOOK_H

#include "board.h"
#include <string>
#include <vector>

namespace Book {
    void init(const std::string& path);
    uint16_t get_move(const Board& board);
    bool is_loaded();
}

#endif
