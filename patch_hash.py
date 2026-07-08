import re

with open('src/board.cpp', 'r') as f:
    content = f.read()

# Make generate_pos_key method
gen_key_code = """
uint64_t Board::generate_pos_key() {
    uint64_t final_key = 0;
    for (int sq = 0; sq < 120; sq++) {
        int piece = pieces[sq];
        if (piece != OFFBOARD && piece != EMPTY) {
            final_key ^= piece_keys[get_piece_index(piece)][sq];
        }
    }
    if (side == BLACK) {
        final_key ^= side_key;
    }
    if (en_passant != SQ_NONE) {
        final_key ^= ep_keys[en_passant];
    }
    final_key ^= castling_keys[castling_rights];
    return final_key;
}
"""

if "uint64_t Board::generate_pos_key()" not in content:
    content = content.replace("Board::Board() {", gen_key_code + "\nBoard::Board() {")

# Add hash_key reset to reset()
if "hash_key = 0;" not in content:
    content = content.replace("history.clear();\n}", "history.clear();\n    hash_key = 0;\n}")

# Add hash_key = generate_pos_key() to set_fen()
if "hash_key = generate_pos_key();" not in content:
    content = content.replace("if (!halfmove_part.empty()) fifty_move = std::stoi(halfmove_part);\n}", "if (!halfmove_part.empty()) fifty_move = std::stoi(halfmove_part);\n    hash_key = generate_pos_key();\n}")

# Fix make_move
make_move_start = """    UndoMove undo;
    undo.move = m;
    undo.captured = captured;
    undo.en_passant = en_passant;
    undo.castling_rights = castling_rights;
    undo.fifty_move = fifty_move;
"""
make_move_start_new = make_move_start + """    undo.hash_key = hash_key;
    
    if (en_passant != SQ_NONE) hash_key ^= ep_keys[en_passant];
    hash_key ^= castling_keys[castling_rights];
"""
content = content.replace(make_move_start, make_move_start_new)

# en passant update in make_move
ep_update = """    en_passant = SQ_NONE;
    if (std::abs(piece) == W_PAWN && std::abs(from - to) == 20) {
        en_passant = (side == WHITE) ? from + 10 : from - 10;
    }"""
ep_update_new = """    en_passant = SQ_NONE;
    if (std::abs(piece) == W_PAWN && std::abs(from - to) == 20) {
        en_passant = (side == WHITE) ? from + 10 : from - 10;
        hash_key ^= ep_keys[en_passant];
    }"""
content = content.replace(ep_update, ep_update_new)

# castling rights update in make_move
castling_update = """    if (from == A1 || to == A1) castling_rights &= ~WQ_CASTLING;
    if (from == H8 || to == H8) castling_rights &= ~BK_CASTLING;
    if (from == A8 || to == A8) castling_rights &= ~BQ_CASTLING;
    """
castling_update_new = castling_update + """
    hash_key ^= castling_keys[castling_rights];
    hash_key ^= side_key; // side switches
    """
content = content.replace(castling_update, castling_update_new)

# Fix unmake_move
unmake_start = """    UndoMove undo = history.back();
    history.pop_back();"""
unmake_start_new = unmake_start + """
    hash_key = undo.hash_key;"""
content = content.replace(unmake_start, unmake_start_new)

with open('src/board.cpp', 'w') as f:
    f.write(content)

