import re

with open('src/board.cpp', 'r') as f:
    content = f.read()

# Delete the PST arrays and update_pst_score in board.cpp
# I'll just use a python script to strip them or I'll overwrite board.cpp.
# Actually, I can just include eval.h in board.cpp.
