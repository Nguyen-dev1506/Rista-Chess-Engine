import re

with open('src/movegen.cpp', 'r') as f:
    content = f.read()

content = content.replace('move_list_list', 'move_list') # fix any double replacement
content = content.replace('move_list.clear();', '')
content = content.replace('move_list.reserve(256);', '')
content = content.replace('move_list.reserve(64);', '')

with open('src/movegen.cpp', 'w') as f:
    f.write(content)

