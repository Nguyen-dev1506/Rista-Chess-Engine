with open("numbfish-main/numbfish.py", "r") as f:
    content = f.read()

content = content.replace("self.use_classical = False", "self.use_classical = (interpreter is None)")

with open("numbfish-main/numbfish.py", "w") as f:
    f.write(content)
