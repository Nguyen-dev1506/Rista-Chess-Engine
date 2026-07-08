import re

with open('src/search.h', 'r') as f:
    content = f.read()

content = content.replace('#include <vector>', '')
content = content.replace('std::vector<Move>& moves', 'MoveList& moves')

with open('src/search.h', 'w') as f:
    f.write(content)

with open('src/search.cpp', 'r') as f:
    content = f.read()

# Replace vector<Move> with MoveList
content = content.replace('std::vector<Move> moves;', 'MoveList moves;')
content = content.replace('std::vector<Move>& moves', 'MoveList& moves')
content = content.replace('moves.size()', 'moves.count')
content = content.replace('for (Move m : moves)', 'for (int _i = 0; _i < moves.count; _i++) {\n        Move m = moves.moves[_i];')

with open('src/search.cpp', 'w') as f:
    f.write(content)
