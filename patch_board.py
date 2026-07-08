import re

with open('src/board.cpp', 'r') as f:
    content = f.read()

# 1. Add include zobrist.h
content = content.replace('#include "board.h"', '#include "board.h"\n#include "zobrist.h"')

# 2. Add generate_pos_key
gen_key_code = """
uint64_t Board::generate_pos_key() {
    uint64_t final_key = 0;
    for (int sq = 0; sq < 120; sq++) {
        int piece = pieces[sq];
        if (piece != OFFBOARD && piece != EMPTY) {
            final_key ^= piece_keys[get_piece_index(piece)][sq];
        }
    }
    if (side == WHITE) {
        final_key ^= side_key;
    }
    if (en_passant != SQ_NONE) {
        final_key ^= ep_keys[en_passant];
    }
    final_key ^= castling_keys[castling_rights];
    return final_key;
}
"""

content = content.replace("Board::Board() {", gen_key_code + "\nBoard::Board() {")

# 3. Add to reset()
content = content.replace("history.clear();\n}", "history.clear();\n    hash_key = 0;\n}")

# 4. Add to set_fen()
content = content.replace("if (!halfmove_part.empty()) fifty_move = std::stoi(halfmove_part);\n}", "if (!halfmove_part.empty()) fifty_move = std::stoi(halfmove_part);\n    hash_key = generate_pos_key();\n}")

# 5. Incremental update in make_move
# It's a bit tricky to patch all incremental updates safely with regex, so I will replace make_move entirely or just use hash_key = generate_pos_key(); for now to ensure correctness, then add incremental. Wait, the user specifically requested incremental XOR.
# Let's write the incremental XOR updates.
# I'll replace make_move and unmake_move using a python script.

