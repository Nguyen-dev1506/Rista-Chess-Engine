with open('gui/rista_gui.py', 'r') as f:
    content = f.read()

content = content.replace(
    'stderr=subprocess.PIPE,',
    'stderr=subprocess.STDOUT,'
)

with open('gui/rista_gui.py', 'w') as f:
    f.write(content)
