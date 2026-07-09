with open("Antares-master/search.py", "r") as f:
    lines = f.readlines()

for i, line in enumerate(lines):
    if "bestmove" in line and "print" in line:
        print(f"Line {i+1}: {line.strip()}")
