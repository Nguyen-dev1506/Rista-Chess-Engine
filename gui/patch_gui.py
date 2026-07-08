import re

with open('rista_gui.py', 'r') as f:
    content = f.read()

# Add UCIEngine class and modify ChessGUI
class_code = """
class UCIEngine:
    def __init__(self, command, name):
        self.name = name
        self.process = subprocess.Popen(
            command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1
        )
        self.queue = queue.Queue()
        self.thread = threading.Thread(target=self._read_output, daemon=True)
        self.thread.start()
        
    def _read_output(self):
        while self.process and self.process.poll() is None:
            try:
                line = self.process.stdout.readline()
                if line:
                    self.queue.put(line.strip())
            except:
                break
                
    def send(self, cmd):
        if self.process and self.process.poll() is None:
            print(f"> [{self.name}] {cmd}")
            self.process.stdin.write(cmd + "\\n")
            self.process.stdin.flush()
            
    def quit(self):
        if self.process:
            try:
                self.send("quit")
                self.process.terminate()
            except:
                pass
"""

content = content.replace("class ChessGUI:", class_code + "\nclass ChessGUI:")

# In __init__:
init_replacement = """        # Game State
        self.board = chess.Board()
        self.selected_sq = None
        self.engine_process = None
        self.engine_queue = queue.Queue()
        self.is_engine_turn = False
        self.move_history = []"""

new_init = """        # Game State
        self.board = chess.Board()
        self.selected_sq = None
        
        self.engine_rista = None
        self.engine_sunfish = None
        
        self.game_mode = "USER_VS_RISTA"
        self.is_engine_turn = False
        self.move_history = []"""

content = content.replace(init_replacement, new_init)

# In setup_ui:
setup_replacement = """        # Controls
        self.new_game_btn = tk.Button(right_panel, text="New Game", command=self.new_game)
        self.new_game_btn.pack(fill=tk.X, pady=(10, 5))"""

new_setup = """        # Controls
        self.new_game_btn = tk.Button(right_panel, text="Play as White vs Rista", command=self.new_game)
        self.new_game_btn.pack(fill=tk.X, pady=(10, 5))
        
        self.eve_btn = tk.Button(right_panel, text="Rista vs Sunfish", command=self.start_eve)
        self.eve_btn.pack(fill=tk.X, pady=(5, 5))"""

content = content.replace(setup_replacement, new_setup)

# Replace start_engine and read_engine_output and send_to_engine and process_engine_queue
engines_code = """
    def start_engine(self):
        try:
            self.engine_rista = UCIEngine(["./rista"], "Rista")
            self.engine_rista.send("uci")
            self.engine_rista.send("isready")
            
            import sys
            self.engine_sunfish = UCIEngine([sys.executable, "sunfish.py"], "Sunfish")
            self.engine_sunfish.send("uci")
            self.engine_sunfish.send("isready")
            
            self.root.after(100, self.process_engine_queues)
        except Exception as e:
            messagebox.showerror("Engine Error", f"Failed to start engines: {e}")

    def process_engine_queues(self):
        if self.engine_rista:
            try:
                while True:
                    msg = self.engine_rista.queue.get_nowait()
                    print(f"< [Rista] {msg}")
                    if msg.startswith("bestmove"):
                        self.handle_bestmove(msg, self.engine_rista)
            except queue.Empty:
                pass
                
        if self.engine_sunfish:
            try:
                while True:
                    msg = self.engine_sunfish.queue.get_nowait()
                    print(f"< [Sunfish] {msg}")
                    if msg.startswith("bestmove"):
                        self.handle_bestmove(msg, self.engine_sunfish)
            except queue.Empty:
                pass
                
        self.root.after(100, self.process_engine_queues)

    def handle_bestmove(self, msg, engine):
        if not self.is_engine_turn:
            return
            
        parts = msg.split()
        if len(parts) >= 2:
            best_move = parts[1]
            if best_move == "(none)" or best_move == "0000":
                self.show_game_over()
                return
                
            try:
                move = chess.Move.from_uci(best_move)
                if move in self.board.legal_moves:
                    self.board.push(move)
                    self.move_history.append(best_move)
                    self.log_move(best_move, engine.name)
                    self.draw_board()
                else:
                    print(f"{engine.name} sent illegal move: {best_move}")
            except Exception as e:
                print(f"Error parsing engine move {best_move}: {e}")
                
        self.is_engine_turn = False
        self.status_label.config(text="Ready", fg="blue")
        
        if self.board.is_game_over():
            self.show_game_over()
        elif self.game_mode == "RISTA_VS_SUNFISH":
            self.root.after(500, self.trigger_next_engine)
            
    def trigger_next_engine(self):
        if self.board.is_game_over(): return
        self.is_engine_turn = True
        self.request_engine_move()
"""

# Regex replacement for start_engine to process_engine_queue
pattern = re.compile(r'    def start_engine\(self\):.*?    def draw_board\(self\):', re.DOTALL)
content = pattern.sub(engines_code + "\n    def draw_board(self):", content)

# update log_move
log_move_old = """    def log_move(self, uci_move):
        self.log_text.config(state=tk.NORMAL)
        turn_num = (len(self.move_history) + 1) // 2
        if len(self.move_history) % 2 != 0:
            self.log_text.insert(tk.END, f"{turn_num}. {uci_move} ")
        else:
            self.log_text.insert(tk.END, f"{uci_move}\\n")
        self.log_text.see(tk.END)
        self.log_text.config(state=tk.DISABLED)"""

log_move_new = """    def log_move(self, uci_move, engine_name=None):
        self.log_text.config(state=tk.NORMAL)
        turn_num = (len(self.move_history) + 1) // 2
        prefix = ""
        if engine_name:
            prefix = f"[{engine_name[0]}] "
        
        if len(self.move_history) % 2 != 0:
            self.log_text.insert(tk.END, f"{turn_num}. {prefix}{uci_move} ")
        else:
            self.log_text.insert(tk.END, f"{prefix}{uci_move}\\n")
        self.log_text.see(tk.END)
        self.log_text.config(state=tk.DISABLED)"""

content = content.replace(log_move_old, log_move_new)

# Update on_canvas_click
canvas_old = """                if move in self.board.legal_moves:
                    self.make_user_move(move)"""

canvas_new = """                if move in self.board.legal_moves:
                    self.make_user_move(move)
                    self.log_move(move.uci(), "User")"""
content = content.replace(canvas_old, canvas_new)

# Update make_user_move
user_move_old = """    def make_user_move(self, move):
        self.board.push(move)
        self.move_history.append(move.uci())
        self.selected_sq = None
        self.log_move(move.uci())
        self.draw_board()
        
        if not self.board.is_game_over():
            self.is_engine_turn = True
            self.request_engine_move()
        else:
            self.show_game_over()"""

user_move_new = """    def make_user_move(self, move):
        self.board.push(move)
        self.move_history.append(move.uci())
        self.selected_sq = None
        self.draw_board()
        
        if not self.board.is_game_over():
            self.is_engine_turn = True
            self.request_engine_move()
        else:
            self.show_game_over()"""
content = content.replace(user_move_old, user_move_new)

# Update request_engine_move
request_old = """    def request_engine_move(self):
        self.status_label.config(text="Engine is thinking...", fg="red")
        self.root.update_idletasks() # Force UI update immediately
        moves_str = " ".join(self.move_history)
        self.send_to_engine(f"position startpos moves {moves_str}")
        self.send_to_engine("go depth 6") # Increased depth!"""

request_new = """    def request_engine_move(self):
        moves_str = " ".join(self.move_history)
        
        if self.game_mode == "USER_VS_RISTA":
            # Rista plays Black
            self.status_label.config(text="Rista is thinking...", fg="red")
            self.root.update_idletasks()
            self.engine_rista.send(f"position startpos moves {moves_str}")
            self.engine_rista.send("go depth 6")
        else:
            # Eve mode
            if self.board.turn == chess.WHITE:
                self.status_label.config(text="Rista is thinking...", fg="red")
                self.root.update_idletasks()
                self.engine_rista.send(f"position startpos moves {moves_str}")
                self.engine_rista.send("go depth 6")
            else:
                self.status_label.config(text="Sunfish is thinking...", fg="red")
                self.root.update_idletasks()
                self.engine_sunfish.send(f"position startpos moves {moves_str}")
                self.engine_sunfish.send("go wtime 30000 btime 30000 winc 0 binc 0")"""
content = content.replace(request_old, request_new)

# Update new_game
new_game_old = """        self.draw_board()
        self.send_to_engine("ucinewgame")"""

new_game_new = """        self.game_mode = "USER_VS_RISTA"
        self.draw_board()
        if self.engine_rista: self.engine_rista.send("ucinewgame")
        if self.engine_sunfish: self.engine_sunfish.send("ucinewgame")
        self.status_label.config(text="Ready (vs Rista)", fg="blue")
        
    def start_eve(self):
        self.board.reset()
        self.move_history.clear()
        self.selected_sq = None
        self.is_engine_turn = True
        self.game_mode = "RISTA_VS_SUNFISH"
        
        self.log_text.config(state=tk.NORMAL)
        self.log_text.delete(1.0, tk.END)
        self.log_text.config(state=tk.DISABLED)
        
        self.draw_board()
        if self.engine_rista: self.engine_rista.send("ucinewgame")
        if self.engine_sunfish: self.engine_sunfish.send("ucinewgame")
        self.status_label.config(text="Rista vs Sunfish Starting...", fg="blue")
        self.root.after(500, self.trigger_next_engine)"""
content = content.replace(new_game_old, new_game_new)

# Update on_close
on_close_old = """    def on_close(self):
        if self.engine_process:
            self.send_to_engine("quit")
            self.engine_process.terminate()
        self.root.destroy()"""

on_close_new = """    def on_close(self):
        if self.engine_rista:
            self.engine_rista.quit()
        if self.engine_sunfish:
            self.engine_sunfish.quit()
        self.root.destroy()"""
content = content.replace(on_close_old, on_close_new)

with open('rista_gui.py', 'w') as f:
    f.write(content)

print("Patched!")
