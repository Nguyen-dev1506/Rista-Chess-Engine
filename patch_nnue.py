import re

with open("numbfish-main/nnue_eval.py", "r") as f:
    content = f.read()

content = content.replace("from tflite_runtime.interpreter import Interpreter", 
"""try:
    from tflite_runtime.interpreter import Interpreter
    interpreter = Interpreter(model_path='numbfish-main/nnue_data/hidden_layers.tflite', num_threads=1)
    interpreter.allocate_tensors()
    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()
except ImportError:
    Interpreter = None
    interpreter = None
    input_details = None
    output_details = None
""")

content = content.replace("interpreter = Interpreter(model_path='nnue_data/hidden_layers.tflite', num_threads=1) #use 1-thread for low latency  ", "")
content = content.replace("interpreter.allocate_tensors()", "")
content = content.replace("input_details = interpreter.get_input_details()", "")
content = content.replace("output_details = interpreter.get_output_details()", "")

with open("numbfish-main/nnue_eval.py", "w") as f:
    f.write(content)
