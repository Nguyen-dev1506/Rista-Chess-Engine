#ifndef UCI_H
#define UCI_H

#include "board.h"
#include <string>

void uci_loop();
void parse_position(Board& board, const std::string& command);
void parse_go(Board& board, const std::string& command);

#endif
