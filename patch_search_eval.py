import re

with open('src/search.cpp', 'r') as f:
    content = f.read()

# Remove evaluate() from search.cpp
eval_pattern = r'int evaluate\(Board& board\) \{.*?\n\}'
content = re.sub(eval_pattern, '', content, flags=re.DOTALL)

# Add #include "eval.h"
if '#include "eval.h"' not in content:
    content = content.replace('#include "search.h"', '#include "search.h"\n#include "eval.h"')

with open('src/search.cpp', 'w') as f:
    f.write(content)

