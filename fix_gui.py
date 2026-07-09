with open('gui/rista_gui.py', 'r') as f:
    content = f.read()

content = content.replace(
    'self.engine_sunfish = UCIEngine([sys.executable, sunfish_path], "Sunfish")',
    'self.engine_sunfish = UCIEngine([sys.executable, "-u", sunfish_path], "Sunfish")'
)

content = content.replace(
    'self.engine_antares = UCIEngine([sys.executable, antares_path], "Antares")',
    'self.engine_antares = UCIEngine([sys.executable, "-u", antares_path], "Antares")'
)

with open('gui/rista_gui.py', 'w') as f:
    f.write(content)
