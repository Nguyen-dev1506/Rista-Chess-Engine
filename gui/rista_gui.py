import tkinter as tk
from tkinter import scrolledtext, messagebox
import subprocess
import threading
import queue
import chess
import time

# UI Constants
CELL_SIZE = 60
BOARD_SIZE = CELL_SIZE * 8
LIGHT_COLOR = "#F0D9B5"
DARK_COLOR = "#B58863"
HIGHLIGHT_COLOR = "#A9A9A9"
MOVE_HIGHLIGHT_COLOR = "#CDD26A"

# Unicode Chess Pieces
PIECE_UNICODE = {
    'P': '♙', 'N': '♘', 'B': '♗', 'R': '♖', 'Q': '♕', 'K': '♔',
    'p': '♟', 'n': '♞', 'b': '♝', 'r': '♜', 'q': '♛', 'k': '♚'
}


class UCIEngine:
    def __init__(self, command, name):
        self.name = name
        self.process = subprocess.Popen(
            command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
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
            self.process.stdin.write(cmd + "\n")
            self.process.stdin.flush()
            
    def quit(self):
        if self.process:
            try:
                self.send("quit")
                self.process.terminate()
            except:
                pass

class ChessGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Rista Chess Engine")
        
        # Game State
        self.board = chess.Board()
        self.selected_sq = None
        
        self.engine_rista = None
        self.engine_sunfish = None
        self.engine_antares = None
        
        self.game_mode = "USER_VS_RISTA"
        self.is_engine_turn = False
        self.move_history = []
        
        # Setup UI
        self.setup_ui()
        
        # Start Engine
        self.start_engine()
        
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

    def setup_ui(self):
        # Main Frame
        main_frame = tk.Frame(self.root)
        main_frame.pack(padx=10, pady=10)
        
        # Board Canvas
        self.canvas = tk.Canvas(main_frame, width=BOARD_SIZE, height=BOARD_SIZE)
        self.canvas.pack(side=tk.LEFT, padx=(0, 10))
        self.canvas.bind("<Button-1>", self.on_canvas_click)
        
        # Right Panel
        right_panel = tk.Frame(main_frame)
        right_panel.pack(side=tk.LEFT, fill=tk.Y)
        
        # Move Log
        tk.Label(right_panel, text="Move Log").pack(anchor=tk.W)
        self.log_text = scrolledtext.ScrolledText(right_panel, width=30, height=20, state=tk.DISABLED)
        self.log_text.pack(fill=tk.Y, pady=(0, 10))
        
        # Controls
        self.new_game_btn = tk.Button(right_panel, text="Play as White vs Rista", command=self.new_game)
        self.new_game_btn.pack(fill=tk.X, pady=(10, 5))
        
        self.eve_btn = tk.Button(right_panel, text="Rista vs Sunfish", command=lambda: self.start_eve("RISTA_VS_SUNFISH"))
        self.eve_btn.pack(fill=tk.X, pady=(5, 5))
        
        self.eve2_btn = tk.Button(right_panel, text="Rista vs Antares", command=lambda: self.start_eve("RISTA_VS_ANTARES"))
        self.eve2_btn.pack(fill=tk.X, pady=(5, 5))
        
        # Status Label
        self.status_label = tk.Label(right_panel, text="Ready", fg="blue", font=("Arial", 12, "bold"))
        self.status_label.pack(fill=tk.X, pady=5)
        
        self.draw_board()


    def start_engine(self):
        import os, sys
        script_dir = os.path.dirname(os.path.abspath(__file__))
        rista_path = os.path.join(script_dir, "..", "rista")
        sunfish_path = os.path.join(script_dir, "sunfish.py")
        antares_path = os.path.join(script_dir, "..", "Antares-master", "main.py")
        try:
            self.engine_rista = UCIEngine([rista_path], "Rista")
            self.engine_rista.send("uci")
            self.engine_rista.send("isready")
            
            self.engine_sunfish = UCIEngine([sys.executable, "-u", sunfish_path], "Sunfish")
            self.engine_sunfish.send("uci")
            self.engine_sunfish.send("isready")
            
            self.status_label.config(text="Khởi động Antares...", fg="orange")
            self.root.update_idletasks()
            self.engine_antares = UCIEngine([sys.executable, "-u", antares_path], "Antares")
            self.engine_antares.send("uci")
            self.engine_antares.send("isready")
            
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
                
        if self.engine_antares:
            try:
                while True:
                    msg = self.engine_antares.queue.get_nowait()
                    print(f"< [Antares] {msg}")
                    if msg.startswith("bestmove"):
                        self.handle_bestmove(msg, self.engine_antares)
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
        elif self.game_mode in ["RISTA_VS_SUNFISH", "RISTA_VS_ANTARES"]:
            self.root.after(500, self.trigger_next_engine)
            
    def trigger_next_engine(self):
        if self.board.is_game_over(): return
        self.is_engine_turn = True
        self.request_engine_move()

    def draw_board(self):
        self.canvas.delete("all")
        
        for rank in range(8):
            for file in range(8):
                x1 = file * CELL_SIZE
                y1 = (7 - rank) * CELL_SIZE
                x2 = x1 + CELL_SIZE
                y2 = y1 + CELL_SIZE
                
                sq = chess.square(file, rank)
                
                # Determine color
                color = LIGHT_COLOR if (rank + file) % 2 != 0 else DARK_COLOR
                
                # Highlights
                if self.selected_sq == sq:
                    color = HIGHLIGHT_COLOR
                elif len(self.board.move_stack) > 0:
                    last_move = self.board.peek()
                    if last_move.from_square == sq or last_move.to_square == sq:
                        color = MOVE_HIGHLIGHT_COLOR
                
                self.canvas.create_rectangle(x1, y1, x2, y2, fill=color, outline="")
                
                # Draw piece
                piece = self.board.piece_at(sq)
                if piece:
                    text_color = "black" if piece.color == chess.BLACK else "white"
                    # Using unicode for pieces
                    symbol = PIECE_UNICODE[piece.symbol()]
                    self.canvas.create_text(
                        x1 + CELL_SIZE/2, y1 + CELL_SIZE/2,
                        text=symbol, font=("Arial", int(CELL_SIZE * 0.7)),
                        fill=text_color
                    )
                    # Add a slight shadow/outline for white pieces to be visible on light squares
                    if piece.color == chess.WHITE:
                        self.canvas.create_text(
                            x1 + CELL_SIZE/2, y1 + CELL_SIZE/2,
                            text=symbol, font=("Arial", int(CELL_SIZE * 0.7)),
                            fill="black"
                        )
                        self.canvas.create_text(
                            x1 + CELL_SIZE/2 - 1, y1 + CELL_SIZE/2 - 1,
                            text=symbol, font=("Arial", int(CELL_SIZE * 0.7)),
                            fill="white"
                        )

    def on_canvas_click(self, event):
        if self.is_engine_turn or self.board.is_game_over():
            return
            
        file = event.x // CELL_SIZE
        rank = 7 - (event.y // CELL_SIZE)
        
        if file < 0 or file > 7 or rank < 0 or rank > 7:
            return
            
        sq = chess.square(file, rank)
        
        if self.selected_sq is None:
            piece = self.board.piece_at(sq)
            if piece and piece.color == self.board.turn:
                self.selected_sq = sq
                self.draw_board()
        else:
            if self.selected_sq == sq:
                self.selected_sq = None
                self.draw_board()
            else:
                move = chess.Move(self.selected_sq, sq)
                
                # Check for promotion
                if self.board.piece_at(self.selected_sq) and self.board.piece_at(self.selected_sq).piece_type == chess.PAWN:
                    if (self.board.turn == chess.WHITE and rank == 7) or (self.board.turn == chess.BLACK and rank == 0):
                        move = chess.Move(self.selected_sq, sq, promotion=chess.QUEEN)
                
                if move in self.board.legal_moves:
                    self.make_user_move(move)
                    self.log_move(move.uci(), "User")
                else:
                    piece = self.board.piece_at(sq)
                    if piece and piece.color == self.board.turn:
                        self.selected_sq = sq
                    else:
                        self.selected_sq = None
                self.draw_board()

    def make_user_move(self, move):
        self.board.push(move)
        self.move_history.append(move.uci())
        self.selected_sq = None
        self.draw_board()
        
        if not self.board.is_game_over():
            self.is_engine_turn = True
            self.request_engine_move()
        else:
            self.show_game_over()

    def request_engine_move(self):
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
                if self.game_mode == "RISTA_VS_SUNFISH":
                    self.status_label.config(text="Sunfish is thinking...", fg="red")
                    self.root.update_idletasks()
                    self.engine_sunfish.send(f"position startpos moves {moves_str}")
                    self.engine_sunfish.send("go wtime 30000 btime 30000 winc 0 binc 0")
                elif self.game_mode == "RISTA_VS_ANTARES":
                    self.status_label.config(text="Antares is thinking...", fg="red")
                    self.root.update_idletasks()
                    self.engine_antares.send(f"position startpos moves {moves_str}")
                    self.engine_antares.send("go depth 6")

    def make_engine_move(self, uci_move):
        try:
            move = chess.Move.from_uci(uci_move)
            if move in self.board.legal_moves:
                self.board.push(move)
                self.move_history.append(uci_move)
                self.log_move(uci_move)
                self.draw_board()
            else:
                print(f"Engine sent illegal move: {uci_move}")
                # Sometimes engine might just output 0000 if it has no moves or something
        except Exception as e:
            print(f"Error parsing engine move {uci_move}: {e}")
            
        self.is_engine_turn = False
        self.status_label.config(text="Ready", fg="blue")
        
        if self.board.is_game_over():
            self.show_game_over()

    def log_move(self, uci_move, engine_name=None):
        self.log_text.config(state=tk.NORMAL)
        turn_num = (len(self.move_history) + 1) // 2
        prefix = ""
        if engine_name:
            prefix = f"[{engine_name[0]}] "
        
        if len(self.move_history) % 2 != 0:
            self.log_text.insert(tk.END, f"{turn_num}. {prefix}{uci_move} ")
        else:
            self.log_text.insert(tk.END, f"{prefix}{uci_move}\n")
        self.log_text.see(tk.END)
        self.log_text.config(state=tk.DISABLED)

    def new_game(self):
        self.board.reset()
        self.move_history.clear()
        self.selected_sq = None
        self.is_engine_turn = False
        
        self.log_text.config(state=tk.NORMAL)
        self.log_text.delete(1.0, tk.END)
        self.log_text.config(state=tk.DISABLED)
        
        self.game_mode = "USER_VS_RISTA"
        self.draw_board()
        if self.engine_rista: self.engine_rista.send("ucinewgame")
        if self.engine_sunfish: self.engine_sunfish.send("ucinewgame")
        self.status_label.config(text="Ready (vs Rista)", fg="blue")
        
    def start_eve(self, mode):
        self.board.reset()
        self.move_history.clear()
        self.selected_sq = None
        self.is_engine_turn = True
        self.game_mode = mode
        
        self.log_text.config(state=tk.NORMAL)
        self.log_text.delete(1.0, tk.END)
        self.log_text.config(state=tk.DISABLED)
        
        self.draw_board()
        if self.engine_rista: self.engine_rista.send("ucinewgame")
        if self.engine_sunfish: self.engine_sunfish.send("ucinewgame")
        if self.engine_antares: self.engine_antares.send("ucinewgame")
        
        name = "Sunfish" if mode == "RISTA_VS_SUNFISH" else "Antares"
        self.status_label.config(text=f"Rista vs {name} Starting...", fg="blue")
        self.root.after(500, self.trigger_next_engine)

    def show_game_over(self):
        result = self.board.result()
        messagebox.showinfo("Game Over", f"Game Over!\nResult: {result}")

    def on_close(self):
        if self.engine_rista:
            self.engine_rista.quit()
        if self.engine_sunfish:
            self.engine_sunfish.quit()
        if self.engine_antares:
            self.engine_antares.quit()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = ChessGUI(root)
    root.mainloop()
