import tkinter as tk
from tkinter import scrolledtext, messagebox
import subprocess
import threading
import queue
import chess
import time
import re
import os
from PIL import Image, ImageTk

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

PIECE_FILE_MAP = {
    'P': 'wP.png', 'N': 'wN.png', 'B': 'wB.png', 'R': 'wR.png', 'Q': 'wQ.png', 'K': 'wK.png',
    'p': 'bP.png', 'n': 'bN.png', 'b': 'bB.png', 'r': 'bR.png', 'q': 'bQ.png', 'k': 'bK.png'
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
        self.drag_data = {"sq": None, "x": 0, "y": 0}
        self.images = {}
        self.load_images()
        
        self.engine_rista = None
        self.engine_sunfish = None
        self.engine_numbfish = None
        self.engine_vice = None
        self.engine_fruit = None
        
        self.game_mode = "USER_VS_RISTA"
        self.is_engine_turn = False
        self.move_history = []
        
        # Clock State
        self.wtime_ms = 0
        self.btime_ms = 0
        self.winc_ms = 0
        self.binc_ms = 0
        self.clock_running = False
        self.last_clock_tick = 0
        self.update_clock_job = None
        
        # Setup UI
        self.setup_ui()
        
        # Start engine polling loop
        self.root.after(100, self.process_engine_queues)
        
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

    def setup_ui(self):
        # Main Frame
        main_frame = tk.Frame(self.root)
        main_frame.pack(padx=10, pady=10)
        # Left Panel (Board + Player Info)
        self.left_panel = tk.Frame(main_frame)
        self.left_panel.pack(side=tk.LEFT, padx=(0, 10))
        
        # Top Player Info
        self.top_player_frame = tk.Frame(self.left_panel, height=40)
        self.top_player_frame.pack(fill=tk.X, pady=(0, 5))
        self.top_name_lbl = tk.Label(self.top_player_frame, text="Opponent", font=("Arial", 12, "bold"))
        self.top_name_lbl.pack(side=tk.LEFT, padx=5)
        self.top_captured_frame = tk.Frame(self.top_player_frame)
        self.top_captured_frame.pack(side=tk.LEFT, padx=5)
        
        # Board Canvas
        self.canvas = tk.Canvas(self.left_panel, width=BOARD_SIZE, height=BOARD_SIZE)
        self.canvas.pack()
        self.canvas.bind("<ButtonPress-1>", self.on_canvas_press)
        self.canvas.bind("<B1-Motion>", self.on_canvas_drag)
        self.canvas.bind("<ButtonRelease-1>", self.on_canvas_release)
        
        # Bottom Player Info
        self.bottom_player_frame = tk.Frame(self.left_panel, height=40)
        self.bottom_player_frame.pack(fill=tk.X, pady=(5, 0))
        self.bottom_name_lbl = tk.Label(self.bottom_player_frame, text="User", font=("Arial", 12, "bold"))
        self.bottom_name_lbl.pack(side=tk.LEFT, padx=5)
        self.bottom_captured_frame = tk.Frame(self.bottom_player_frame)
        self.bottom_captured_frame.pack(side=tk.LEFT, padx=5)
        
        # Right Panel
        right_panel = tk.Frame(main_frame)
        right_panel.pack(side=tk.LEFT, fill=tk.Y, padx=(10, 0))
        
        # Move Log
        lbl_frame = tk.Frame(right_panel)
        lbl_frame.pack(fill=tk.X)
        tk.Label(lbl_frame, text="Move Log").pack(side=tk.LEFT)
        tk.Button(lbl_frame, text="Export PGN", command=self.export_pgn).pack(side=tk.RIGHT)
        
        self.log_text = scrolledtext.ScrolledText(right_panel, width=30, height=12, state=tk.DISABLED)
        self.log_text.pack(fill=tk.Y, pady=(0, 10))
        
        # Controls
        tk.Label(right_panel, text="Rista Color").pack(anchor=tk.W, pady=(5,0))
        self.rista_color_var = tk.StringVar(value="White")
        tk.Radiobutton(right_panel, text="White", variable=self.rista_color_var, value="White").pack(anchor=tk.W)
        tk.Radiobutton(right_panel, text="Black", variable=self.rista_color_var, value="Black").pack(anchor=tk.W)
        
        self.flip_board_var = tk.BooleanVar(value=False)
        tk.Checkbutton(right_panel, text="Flip Board", variable=self.flip_board_var, command=self.draw_board).pack(anchor=tk.W, pady=(5,0))
        
        tk.Label(right_panel, text="Game Modes").pack(anchor=tk.W, pady=(10,0))
        self.new_game_btn = tk.Button(right_panel, text="User vs Rista", command=self.start_user_mode)
        self.new_game_btn.pack(fill=tk.X, pady=2)
        
        self.eve_btn = tk.Button(right_panel, text="Rista vs Sunfish", command=lambda: self.start_eve("Sunfish"))
        self.eve_btn.pack(fill=tk.X, pady=2)
        
        self.eve2_btn = tk.Button(right_panel, text="Rista vs Numbfish", command=lambda: self.start_eve("Numbfish"))
        self.eve2_btn.pack(fill=tk.X, pady=2)
        
        self.eve3_btn = tk.Button(right_panel, text="Rista vs Vice", command=lambda: self.start_eve("Vice"))
        self.eve3_btn.pack(fill=tk.X, pady=2)
        
        self.eve4_btn = tk.Button(right_panel, text="Rista vs Fruit", command=lambda: self.start_eve("Fruit"))
        self.eve4_btn.pack(fill=tk.X, pady=2)
        
        self.eve5_btn = tk.Button(right_panel, text="Rista 3.3 vs Rista 3.0", command=self.start_rista_3_0_match)
        self.eve5_btn.pack(fill=tk.X, pady=2)

        
        self.log_btn = tk.Button(right_panel, text="Load Game Log", command=self.open_log_viewer)
        self.log_btn.pack(fill=tk.X, pady=(10, 2))
        
        # Status Label
        self.status_label = tk.Label(right_panel, text="Ready", fg="blue", font=("Arial", 12, "bold"))
        self.status_label.pack(fill=tk.X, pady=5)
        
        self.draw_board()
        
    def export_pgn(self):
        import chess.pgn
        game = chess.pgn.Game()
        game.headers["Event"] = "Rista Chess Match"
        game.headers["White"] = getattr(self, 'white_player', 'User')
        game.headers["Black"] = getattr(self, 'black_player', 'User')
        game.headers["Result"] = self.board.result()
        
        node = game
        temp_board = chess.Board()
        for uci in self.move_history:
            try:
                move = chess.Move.from_uci(uci)
                if move in temp_board.legal_moves:
                    node = node.add_variation(move)
                    temp_board.push(move)
            except:
                pass
                
        pgn_text = str(game)
        
        top = tk.Toplevel(self.root)
        top.title("Export PGN")
        
        text_area = scrolledtext.ScrolledText(top, width=50, height=20)
        text_area.pack(padx=10, pady=10)
        text_area.insert(tk.END, pgn_text)
        
        def copy_to_clipboard():
            self.root.clipboard_clear()
            self.root.clipboard_append(pgn_text)
            self.root.update()
            messagebox.showinfo("Copied", "PGN has been copied to clipboard!", parent=top)
            
        tk.Button(top, text="Copy to Clipboard", command=copy_to_clipboard).pack(pady=5)

    def load_images(self):
        script_dir = os.path.dirname(os.path.abspath(__file__))
        img_dir = os.path.join(script_dir, "assets", "pngs")
        self.mini_images = {}
        for symbol, filename in PIECE_FILE_MAP.items():
            path = os.path.join(img_dir, filename)
            if os.path.exists(path):
                img = Image.open(path)
                large_img = img.resize((int(CELL_SIZE*0.9), int(CELL_SIZE*0.9)), Image.Resampling.LANCZOS)
                self.images[symbol] = ImageTk.PhotoImage(large_img)
                
                mini_img = img.resize((25, 25), Image.Resampling.LANCZOS)
                self.mini_images[symbol] = ImageTk.PhotoImage(mini_img)

    def open_log_viewer(self):
        LogViewer(self.root, self.images)


    def quit_all_engines(self):
        for attr in ['engine_rista', 'engine_rista_legacy', 'engine_sunfish', 'engine_numbfish', 'engine_vice', 'engine_fruit']:
            engine = getattr(self, attr, None)
            if engine:
                engine.quit()
                setattr(self, attr, None)

    def setup_engines_for_match(self, opponent=None):
        self.quit_all_engines()
        import os, sys
        script_dir = os.path.dirname(os.path.abspath(__file__))
        
        try:
            # Always start Rista
            rista_path = os.path.join(script_dir, "..", "rista")
            self.engine_rista = UCIEngine([rista_path], "Rista")
            self.engine_rista.send("uci")
            self.engine_rista.send("isready")
            
            if opponent == "Sunfish":
                sunfish_path = os.path.join(script_dir, "sunfish.py")
                self.engine_sunfish = UCIEngine([sys.executable, "-u", sunfish_path], "Sunfish")
                self.engine_sunfish.send("uci")
                self.engine_sunfish.send("isready")
            elif opponent == "Numbfish":
                numbfish_path = os.path.join(script_dir, "..", "opponents", "numbfish-main", "uci.py")
                self.engine_numbfish = UCIEngine([sys.executable, "-u", numbfish_path], "Numbfish")
                self.engine_numbfish.send("uci")
                self.engine_numbfish.send("isready")
            elif opponent == "Vice":
                vice_path = os.path.join(script_dir, "..", "opponents", "vice-main", "Vice11", "src", "vice12_smp")
                self.engine_vice = UCIEngine([vice_path], "Vice")
                self.engine_vice.send("uci")
                self.engine_vice.send("isready")
            elif opponent == "Fruit":
                fruit_path = os.path.join(script_dir, "..", "opponents", "Fruit-2.1-master", "src", "fruit")
                self.engine_fruit = UCIEngine([fruit_path], "Fruit")
                self.engine_fruit.send("uci")
                self.engine_fruit.send("isready")
            elif opponent == "Rista 3.0":
                rista_legacy_path = os.path.join(script_dir, "..", "rista_3_0")
                self.engine_rista_legacy = UCIEngine([rista_legacy_path], "Rista 3.0")
                self.engine_rista_legacy.send("uci")
                self.engine_rista_legacy.send("isready")
        
        except Exception as e:
            messagebox.showerror("Engine Error", f"Failed to start engines: {e}")

    def process_engine_queues(self):
        if self.engine_rista:
            try:
                while self.engine_rista:
                    msg = self.engine_rista.queue.get_nowait()
                    print(f"< [Rista] {msg}")
                    if msg.startswith("bestmove"):
                        self.handle_bestmove(msg, self.engine_rista)
            except queue.Empty:
                pass
                
        if self.engine_sunfish:
            try:
                while self.engine_sunfish:
                    msg = self.engine_sunfish.queue.get_nowait()
                    print(f"< [Sunfish] {msg}")
                    if msg.startswith("bestmove"):
                        self.handle_bestmove(msg, self.engine_sunfish)
            except queue.Empty:
                pass
                
        if self.engine_numbfish:
            try:
                while self.engine_numbfish:
                    msg = self.engine_numbfish.queue.get_nowait()
                    print(f"< [Numbfish] {msg}")
                    if msg.startswith("bestmove"):
                        self.handle_bestmove(msg, self.engine_numbfish)
            except queue.Empty:
                pass
                
        if self.engine_vice:
            try:
                while self.engine_vice:
                    msg = self.engine_vice.queue.get_nowait()
                    print(f"< [Vice] {msg}")
                    if msg.startswith("bestmove"):
                        self.handle_bestmove(msg, self.engine_vice)
            except queue.Empty:
                pass
                
        if self.engine_fruit:
            try:
                while self.engine_fruit:
                    msg = self.engine_fruit.queue.get_nowait()
                    print(f"< [Fruit] {msg}")
                    if msg.startswith("bestmove"):
                        self.handle_bestmove(msg, self.engine_fruit)
            except queue.Empty:
                pass
                
        if hasattr(self, 'engine_rista_legacy') and self.engine_rista_legacy:
            try:
                while self.engine_rista_legacy:
                    msg = self.engine_rista_legacy.queue.get_nowait()
                    print(f"< [Rista 3.0] {msg}")
                    if msg.startswith("bestmove"):
                        self.handle_bestmove(msg, self.engine_rista_legacy)
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
        elif hasattr(self, 'white_player') and hasattr(self, 'black_player'):
            # Trigger next engine if it's an engine's turn
            current_player = self.white_player if self.board.turn == chess.WHITE else self.black_player
            if current_player != "User":
                self.root.after(500, self.trigger_next_engine)
            
    def trigger_next_engine(self):
        if self.board.is_game_over(): return
        self.is_engine_turn = True
        self.request_engine_move()

    def update_player_info(self):
        flip = hasattr(self, 'flip_board_var') and self.flip_board_var.get()
        
        white_player = getattr(self, 'white_player', 'User (White)')
        black_player = getattr(self, 'black_player', 'Rista (Black)')
        
        if flip:
            self.bottom_name_lbl.config(text=black_player)
            self.top_name_lbl.config(text=white_player)
            bottom_color = chess.BLACK
            top_color = chess.WHITE
        else:
            self.bottom_name_lbl.config(text=white_player)
            self.top_name_lbl.config(text=black_player)
            bottom_color = chess.WHITE
            top_color = chess.BLACK
            
        initial_pieces = {
            chess.PAWN: 8, chess.KNIGHT: 2, chess.BISHOP: 2, chess.ROOK: 2, chess.QUEEN: 1
        }
        
        current_pieces = {chess.WHITE: {}, chess.BLACK: {}}
        for pt in initial_pieces:
            current_pieces[chess.WHITE][pt] = len(self.board.pieces(pt, chess.WHITE))
            current_pieces[chess.BLACK][pt] = len(self.board.pieces(pt, chess.BLACK))
            
        def get_captured_for(opp_color):
            captured = []
            for pt in [chess.PAWN, chess.KNIGHT, chess.BISHOP, chess.ROOK, chess.QUEEN]:
                missing = initial_pieces[pt] - current_pieces[opp_color][pt]
                for _ in range(missing):
                    captured.append((pt, opp_color))
            return captured
            
        bottom_captured = get_captured_for(top_color)
        top_captured = get_captured_for(bottom_color)
        
        self.draw_captured_pieces(self.bottom_captured_frame, bottom_captured)
        self.draw_captured_pieces(self.top_captured_frame, top_captured)
        
    def draw_captured_pieces(self, frame, captured_list):
        for widget in frame.winfo_children():
            widget.destroy()
            
        for pt, color in captured_list:
            symbol = chess.piece_symbol(pt)
            if color == chess.WHITE: symbol = symbol.upper()
            
            if symbol in self.mini_images:
                lbl = tk.Label(frame, image=self.mini_images[symbol])
                lbl.pack(side=tk.LEFT, padx=1)

    def draw_board(self):
        self.update_player_info()
        self.canvas.delete("all")
        flip = hasattr(self, 'flip_board_var') and self.flip_board_var.get()
        
        valid_moves = []
        if self.selected_sq is not None:
            valid_moves = [m.to_square for m in self.board.legal_moves if m.from_square == self.selected_sq]
        
        for rank in range(8):
            for file in range(8):
                draw_file = 7 - file if flip else file
                draw_rank = 7 - rank if flip else rank
                
                x1 = draw_file * CELL_SIZE
                y1 = (7 - draw_rank) * CELL_SIZE
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
                
                # Draw valid move indicators
                if sq in valid_moves:
                    cx = x1 + CELL_SIZE/2
                    cy = y1 + CELL_SIZE/2
                    if self.board.piece_at(sq):
                        self.canvas.create_oval(x1+4, y1+4, x2-4, y2-4, outline="#888888", width=5)
                    else:
                        r = CELL_SIZE * 0.15
                        self.canvas.create_oval(cx-r, cy-r, cx+r, cy+r, fill="#888888", outline="")
                
                # Draw piece
                piece = self.board.piece_at(sq)
                if piece:
                    symbol = piece.symbol()
                    if symbol in self.images:
                        self.canvas.create_image(
                            x1 + CELL_SIZE/2, y1 + CELL_SIZE/2,
                            image=self.images[symbol],
                            tags=f"piece_{sq}"
                        )
                    else:
                        text_color = "black" if piece.color == chess.BLACK else "white"
                        unicode_sym = PIECE_UNICODE[symbol]
                        self.canvas.create_text(
                            x1 + CELL_SIZE/2, y1 + CELL_SIZE/2,
                            text=unicode_sym, font=("Arial", int(CELL_SIZE * 0.7)),
                            fill=text_color, tags=f"piece_{sq}"
                        )

    def on_canvas_press(self, event):
        if self.is_engine_turn or self.board.is_game_over():
            return
            
        file = event.x // CELL_SIZE
        rank = 7 - (event.y // CELL_SIZE)
        
        flip = hasattr(self, 'flip_board_var') and self.flip_board_var.get()
        if flip:
            file = 7 - file
            rank = 7 - rank
        
        if file < 0 or file > 7 or rank < 0 or rank > 7:
            return
            
        sq = chess.square(file, rank)
        
        piece = self.board.piece_at(sq)
        if piece and piece.color == self.board.turn:
            self.selected_sq = sq
            self.drag_data["sq"] = sq
            self.drag_data["x"] = event.x
            self.drag_data["y"] = event.y
            self.draw_board()
            self.canvas.tag_raise(f"piece_{sq}")
        else:
            if self.selected_sq is not None:
                move = chess.Move(self.selected_sq, sq)
                if self.board.piece_at(self.selected_sq) and self.board.piece_at(self.selected_sq).piece_type == chess.PAWN:
                    if (self.board.turn == chess.WHITE and rank == 7) or (self.board.turn == chess.BLACK and rank == 0):
                        move = chess.Move(self.selected_sq, sq, promotion=chess.QUEEN)
                
                if move in self.board.legal_moves:
                    self.make_user_move(move)
                    self.log_move(move.uci(), "User")
                else:
                    self.selected_sq = None
                self.draw_board()

    def on_canvas_drag(self, event):
        if self.drag_data["sq"] is not None:
            dx = event.x - self.drag_data["x"]
            dy = event.y - self.drag_data["y"]
            self.canvas.move(f"piece_{self.drag_data['sq']}", dx, dy)
            self.drag_data["x"] = event.x
            self.drag_data["y"] = event.y

    def on_canvas_release(self, event):
        if self.drag_data["sq"] is not None:
            file = event.x // CELL_SIZE
            rank = 7 - (event.y // CELL_SIZE)
            
            flip = hasattr(self, 'flip_board_var') and self.flip_board_var.get()
            if flip:
                file = 7 - file
                rank = 7 - rank
                
            sq = chess.square(file, rank)
            if 0 <= file <= 7 and 0 <= rank <= 7 and sq != self.drag_data["sq"]:
                move = chess.Move(self.drag_data["sq"], sq)
                if self.board.piece_at(self.drag_data["sq"]) and self.board.piece_at(self.drag_data["sq"]).piece_type == chess.PAWN:
                    if (self.board.turn == chess.WHITE and rank == 7) or (self.board.turn == chess.BLACK and rank == 0):
                        move = chess.Move(self.drag_data["sq"], sq, promotion=chess.QUEEN)
                
                if move in self.board.legal_moves:
                    self.make_user_move(move)
                    self.log_move(move.uci(), "User")
                else:
                    self.selected_sq = None
                    self.draw_board()
            else:
                self.draw_board()
            
            self.drag_data["sq"] = None

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
        current_player = self.white_player if self.board.turn == chess.WHITE else self.black_player
        
        if current_player == "User":
            self.is_engine_turn = False
            self.status_label.config(text="Your turn", fg="green")
            return
            
        self.status_label.config(text=f"{current_player} is thinking...", fg="red")
        self.root.update_idletasks()
        
        # NOTE: Rista uses movetime 1000, other engines use depth
        
        if current_player == "Rista":
            if self.engine_rista:
                self.engine_rista.send(f"position startpos moves {moves_str}")
                self.engine_rista.send("go movetime 1000")
            else:
                self.status_label.config(text="Rista engine not found!", fg="red")
                self.is_engine_turn = False
        elif current_player == "Sunfish":
            if self.engine_sunfish:
                self.engine_sunfish.send(f"position startpos moves {moves_str}")
                self.engine_sunfish.send("go wtime 30000 btime 30000 winc 0 binc 0")
            else:
                self.status_label.config(text="Sunfish engine not found!", fg="red")
                self.is_engine_turn = False
        elif current_player == "Numbfish":
            if self.engine_numbfish:
                self.engine_numbfish.send(f"position startpos moves {moves_str}")
                self.engine_numbfish.send("go depth 6")
            else:
                self.status_label.config(text="Numbfish engine not found!", fg="red")
                self.is_engine_turn = False
        elif current_player == "Vice":
            if self.engine_vice:
                self.engine_vice.send(f"position startpos moves {moves_str}")
                self.engine_vice.send("go depth 7")
            else:
                self.status_label.config(text="Vice engine not found!", fg="red")
                self.is_engine_turn = False
        elif current_player == "Fruit":
            if self.engine_fruit:
                self.engine_fruit.send(f"position startpos moves {moves_str}")
                self.engine_fruit.send("go depth 7")
            else:
                self.status_label.config(text="Fruit engine not found!", fg="red")
                self.is_engine_turn = False
        elif current_player == "Rista 3.0":
            if hasattr(self, 'engine_rista_legacy') and self.engine_rista_legacy:
                self.engine_rista_legacy.send(f"position startpos moves {moves_str}")
                self.engine_rista_legacy.send("go movetime 1000")
            else:
                self.status_label.config(text="Rista 3.0 engine not found!", fg="red")
                self.is_engine_turn = False

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

    def start_user_mode(self):
        self.status_label.config(text="Khởi động engine...", fg="orange")
        self.root.update_idletasks()
        self.setup_engines_for_match(None)

        is_white = (self.rista_color_var.get() == "White")
        white = "Rista" if is_white else "User"
        black = "User" if is_white else "Rista"
        
        # Auto-flip if User is Black
        self.flip_board_var.set(not is_white)
        
        self.new_game(white, black)

    def new_game(self, white, black):
        self.board.reset()
        self.move_history.clear()
        self.selected_sq = None
        self.is_engine_turn = False
        
        self.log_text.config(state=tk.NORMAL)
        self.log_text.delete(1.0, tk.END)
        self.log_text.config(state=tk.DISABLED)
        
        self.white_player = white
        self.black_player = black
        
        self.draw_board()
        if self.engine_rista: self.engine_rista.send("ucinewgame")
        if self.engine_sunfish: self.engine_sunfish.send("ucinewgame")
        if self.engine_numbfish: self.engine_numbfish.send("ucinewgame")
        if self.engine_vice: self.engine_vice.send("ucinewgame")
        if self.engine_fruit: self.engine_fruit.send("ucinewgame")
        if hasattr(self, 'engine_rista_legacy') and self.engine_rista_legacy: self.engine_rista_legacy.send("ucinewgame")
        
        self.status_label.config(text=f"Ready ({white} vs {black})", fg="blue")
        if white != "User":
            self.is_engine_turn = True
            self.request_engine_move()
        
    def start_eve(self, opponent):
        self.status_label.config(text=f"Khởi động {opponent}...", fg="orange")
        self.root.update_idletasks()
        self.setup_engines_for_match(opponent)

        is_white = (self.rista_color_var.get() == "White")
        white = "Rista" if is_white else opponent
        black = opponent if is_white else "Rista"
        
        # Auto-flip if Rista is Black
        self.flip_board_var.set(not is_white)
        
        import random
        self.board.reset()
        self.move_history.clear()
        
        self.log_text.config(state=tk.NORMAL)
        self.log_text.delete(1.0, tk.END)
        self.log_text.config(state=tk.DISABLED)

        openings = [
            [],
            ["e2e4", "e7e5"],
            ["e2e4", "c7c5"],
            ["d2d4", "d7d5"],
            ["d2d4", "g8f6"],
            ["e2e4", "e7e6"],
            ["c2c4", "e7e5"],
            ["g1f3", "d7d5"],
            ["e2e4", "c7c6"],
            ["d2d4", "f7f5"],
            ["b2b3", "e7e5"],
            ["f2f4", "d7d5"],
            ["e2e4", "d7d6"],
            ["e2e4", "g7g6"],
            ["d2d4", "g8f6", "c2c4", "g7g6"],
            ["e2e4", "e7e5", "g1f3", "b8c6", "f1c4"],
            ["d2d4", "d7d5", "c2c4", "c7c6"]
        ]
        chosen = random.choice(openings)
        for m in chosen:
            move = chess.Move.from_uci(m)
            self.board.push(move)
            self.move_history.append(m)
            self.log_move(m, "Book")

        self.selected_sq = None
        self.white_player = white
        self.black_player = black
        
        self.draw_board()
        if self.engine_rista: self.engine_rista.send("ucinewgame")
        if self.engine_sunfish: self.engine_sunfish.send("ucinewgame")
        if self.engine_numbfish: self.engine_numbfish.send("ucinewgame")
        if self.engine_vice: self.engine_vice.send("ucinewgame")
        if self.engine_fruit: self.engine_fruit.send("ucinewgame")
        if hasattr(self, 'engine_rista_legacy') and self.engine_rista_legacy: self.engine_rista_legacy.send("ucinewgame")
        
        self.status_label.config(text=f"{white} vs {black} Starting...", fg="blue")
        self.is_engine_turn = True
        self.root.after(500, self.trigger_next_engine)

    def start_rista_3_0_match(self):
        self.status_label.config(text="Khởi động Rista 3.0...", fg="orange")
        self.root.update_idletasks()
        self.setup_engines_for_match("Rista 3.0")

        is_rista3_white = (self.rista_color_var.get() == "White")
        white = "Rista" if is_rista3_white else "Rista 3.0"
        black = "Rista 3.0" if is_rista3_white else "Rista"
        
        self.flip_board_var.set(not is_rista3_white)
        
        self.board.reset()
        self.move_history.clear()
        
        self.log_text.config(state=tk.NORMAL)
        self.log_text.delete(1.0, tk.END)
        self.log_text.config(state=tk.DISABLED)

        self.selected_sq = None
        self.white_player = white
        self.black_player = black
        
        self.draw_board()
        if self.engine_rista: self.engine_rista.send("ucinewgame")
        if hasattr(self, 'engine_rista_legacy') and self.engine_rista_legacy: self.engine_rista_legacy.send("ucinewgame")
        
        self.status_label.config(text=f"{white} vs {black} Starting...", fg="blue")
        self.is_engine_turn = True
        self.root.after(500, self.trigger_next_engine)

    def show_game_over(self):
        result = self.board.result()
        if result == "1-0":
            winner = getattr(self, 'white_player', 'White')
            msg = f"Trắng ({winner}) THẮNG!"
        elif result == "0-1":
            winner = getattr(self, 'black_player', 'Black')
            msg = f"Đen ({winner}) THẮNG!"
        else:
            msg = "HÒA (Draw)!"
            
        self.quit_all_engines()
        messagebox.showinfo("Game Over", f"Trận đấu kết thúc!\nKết quả: {result}\n\n{msg}")

    def on_close(self):
        self.quit_all_engines()
        self.root.destroy()

class LogViewer:
    def __init__(self, parent, images):
        self.images = images
        self.top = tk.Toplevel(parent)
        self.top.title("Game Log Viewer")
        
        self.board = chess.Board()
        self.moves = []
        self.current_ply = 0
        
        main_frame = tk.Frame(self.top)
        main_frame.pack(padx=10, pady=10)
        
        left_panel = tk.Frame(main_frame)
        left_panel.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 10))
        
        tk.Label(left_panel, text="Paste Game Log Here:").pack(anchor=tk.W)
        self.text_area = scrolledtext.ScrolledText(left_panel, width=40, height=15)
        self.text_area.pack(pady=5)
        
        tk.Button(left_panel, text="Parse & Load Moves", command=self.load_moves).pack(fill=tk.X)
        
        nav_frame = tk.Frame(left_panel)
        nav_frame.pack(pady=10)
        
        tk.Button(nav_frame, text="|<<", width=4, command=self.go_start).pack(side=tk.LEFT, padx=2)
        tk.Button(nav_frame, text="<", width=4, command=self.go_prev).pack(side=tk.LEFT, padx=2)
        tk.Button(nav_frame, text=">", width=4, command=self.go_next).pack(side=tk.LEFT, padx=2)
        tk.Button(nav_frame, text=">>|", width=4, command=self.go_end).pack(side=tk.LEFT, padx=2)
        
        self.status_lbl = tk.Label(left_panel, text="Ready")
        self.status_lbl.pack(pady=5)
        
        self.canvas = tk.Canvas(main_frame, width=BOARD_SIZE, height=BOARD_SIZE)
        self.canvas.pack(side=tk.LEFT)
        
        self.draw_board()
        
    def load_moves(self):
        text = self.text_area.get(1.0, tk.END)
        tokens = re.findall(r'\b[a-h][1-8][a-h][1-8][qrbn]?\b', text)
        
        self.board.reset()
        self.moves = []
        self.current_ply = 0
        
        valid_count = 0
        for token in tokens:
            try:
                move = chess.Move.from_uci(token)
                if move in self.board.legal_moves:
                    self.board.push(move)
                    self.moves.append(move)
                    valid_count += 1
                else:
                    break
            except:
                pass
                
        self.board.reset()
        self.current_ply = 0
        self.draw_board()
        self.status_lbl.config(text=f"Loaded {valid_count} moves.")
        
    def go_start(self):
        while self.current_ply > 0:
            self.go_prev()
            
    def go_end(self):
        while self.current_ply < len(self.moves):
            self.go_next()
            
    def go_prev(self):
        if self.current_ply > 0:
            self.board.pop()
            self.current_ply -= 1
            self.draw_board()
            
    def go_next(self):
        if self.current_ply < len(self.moves):
            self.board.push(self.moves[self.current_ply])
            self.current_ply += 1
            self.draw_board()
            
    def draw_board(self):
        self.canvas.delete("all")
        last_move = None
        if len(self.board.move_stack) > 0:
            last_move = self.board.peek()
            
        for rank in range(8):
            for file in range(8):
                x1 = file * CELL_SIZE
                y1 = (7 - rank) * CELL_SIZE
                x2 = x1 + CELL_SIZE
                y2 = y1 + CELL_SIZE
                sq = chess.square(file, rank)
                
                color = LIGHT_COLOR if (rank + file) % 2 != 0 else DARK_COLOR
                if last_move and (last_move.from_square == sq or last_move.to_square == sq):
                    color = MOVE_HIGHLIGHT_COLOR
                    
                self.canvas.create_rectangle(x1, y1, x2, y2, fill=color, outline="")
                
                piece = self.board.piece_at(sq)
                if piece:
                    symbol = piece.symbol()
                    if hasattr(self, 'images') and symbol in self.images:
                        self.canvas.create_image(
                            x1 + CELL_SIZE/2, y1 + CELL_SIZE/2,
                            image=self.images[symbol]
                        )
                    else:
                        text_color = "black" if piece.color == chess.BLACK else "white"
                        unicode_sym = PIECE_UNICODE[symbol]
                        if piece.color == chess.WHITE:
                            self.canvas.create_text(x1 + CELL_SIZE/2, y1 + CELL_SIZE/2, text=unicode_sym, font=("Arial", int(CELL_SIZE * 0.7)), fill="black")
                            self.canvas.create_text(x1 + CELL_SIZE/2 - 1, y1 + CELL_SIZE/2 - 1, text=unicode_sym, font=("Arial", int(CELL_SIZE * 0.7)), fill="white")
                        else:
                            self.canvas.create_text(x1 + CELL_SIZE/2, y1 + CELL_SIZE/2, text=unicode_sym, font=("Arial", int(CELL_SIZE * 0.7)), fill=text_color)

if __name__ == "__main__":
    root = tk.Tk()
    root.geometry("800x650")
    
    app = ChessGUI(root)
    
    logo_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "logo.png")
    if os.path.exists(logo_path):
        img = Image.open(logo_path).resize((520, 520), Image.Resampling.LANCZOS)
        splash_img = ImageTk.PhotoImage(img)
        root.splash_img = splash_img
        
        splash_label = tk.Label(root, image=splash_img, borderwidth=0, bg="#89C4F4")
        splash_label.place(x=0, y=0, relwidth=1, relheight=1)
        splash_label.lift()
        
        root.after(2000, splash_label.destroy)
        
    root.mainloop()
