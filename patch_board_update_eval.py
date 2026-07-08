import re

with open('src/board.cpp', 'r') as f:
    content = f.read()

# Add include eval.h
if '#include "eval.h"' not in content:
    content = content.replace('#include "board.h"', '#include "board.h"\n#include "eval.h"')

# Replace Board::reset initializing pst_score to initializing mg_score, eg_score, game_phase
reset_old = """    material_score = 0;
    pst_score = 0;"""
reset_new = """    mg_score[0] = 0; mg_score[1] = 0;
    eg_score[0] = 0; eg_score[1] = 0;
    game_phase = 0;"""
content = content.replace(reset_old, reset_new)

# Replace update_pst_score implementation to use update_eval
update_old_pattern = r'void Board::update_pst_score\(int piece, int sq, bool is_add\) \{.*?hash_key \^= piece_keys\[get_piece_index\(piece\)\]\[sq\];\n\}'
update_new = """void Board::update_pst_score(int piece, int sq, bool is_add) {
    update_eval(*this, piece, sq, is_add);
    hash_key ^= piece_keys[get_piece_index(piece)][sq];
}"""
content = re.sub(update_old_pattern, update_new, content, flags=re.DOTALL)

with open('src/board.cpp', 'w') as f:
    f.write(content)

