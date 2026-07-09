import subprocess
import threading
import queue
import chess
import random
import sys
import os
import concurrent.futures

openings = [
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
            self.process.stdin.write(cmd + "\n")
            self.process.stdin.flush()
            
    def get_bestmove(self, timeout=30):
        try:
            while True:
                msg = self.queue.get(timeout=timeout)
                if msg.startswith("bestmove"):
                    parts = msg.split()
                    if len(parts) >= 2:
                        return parts[1]
        except queue.Empty:
            return None
            
    def quit(self):
        if self.process:
            try:
                self.send("quit")
                self.process.terminate()
                self.process.wait(timeout=2)
            except:
                self.process.kill()

def play_match(match_id):
    script_dir = os.path.dirname(os.path.abspath(__file__))
    rista_path = os.path.join(script_dir, "rista")
    sunfish_path = os.path.join(script_dir, "gui", "sunfish.py")
    
    # Alternate colors for Rista (even matches play White, odd matches play Black)
    rista_is_white = (match_id % 2 == 0)
    
    engine_w = UCIEngine([rista_path], f"Rista (Match {match_id})") if rista_is_white else UCIEngine([sys.executable, "-u", sunfish_path], f"Sunfish (Match {match_id})")
    engine_b = UCIEngine([sys.executable, "-u", sunfish_path], f"Sunfish (Match {match_id})") if rista_is_white else UCIEngine([rista_path], f"Rista (Match {match_id})")
    
    engine_w.send("uci")
    engine_b.send("uci")
    
    board = chess.Board()
    move_history = []
    
    # Apply opening book
    chosen = random.choice(openings)
    for m in chosen:
        board.push(chess.Move.from_uci(m))
        move_history.append(m)
        
    engine_w.send("ucinewgame")
    engine_b.send("ucinewgame")
    
    print(f"[Match {match_id}] Started. Rista is {'White' if rista_is_white else 'Black'}. Opening: {' '.join(chosen)}")
    
    result = "*"
    
    while not board.is_game_over() and len(move_history) < 150:
        moves_str = " ".join(move_history)
        cmd_pos = f"position startpos moves {moves_str}" if moves_str else "position startpos"
        
        if board.turn == chess.WHITE:
            engine_w.send(cmd_pos)
            if rista_is_white:
                engine_w.send("go depth 8")
            else:
                engine_w.send("go wtime 10000 btime 10000 winc 0 binc 0")
            bestmove = engine_w.get_bestmove()
            actor = "Rista (W)" if rista_is_white else "Sunfish (W)"
        else:
            engine_b.send(cmd_pos)
            if not rista_is_white:
                engine_b.send("go depth 8")
            else:
                engine_b.send("go wtime 10000 btime 10000 winc 0 binc 0")
            bestmove = engine_b.get_bestmove()
            actor = "Sunfish (B)" if rista_is_white else "Rista (B)"
            
        if not bestmove or bestmove == "(none)" or bestmove == "0000":
            break
            
        try:
            move = chess.Move.from_uci(bestmove)
            if move in board.legal_moves:
                board.push(move)
                move_history.append(bestmove)
            else:
                print(f"[Match {match_id}] {actor} sent illegal move: {bestmove}")
                result = "0-1" if board.turn == chess.WHITE else "1-0"
                break
        except Exception as e:
            print(f"[Match {match_id}] {actor} exception: {e}")
            result = "0-1" if board.turn == chess.WHITE else "1-0"
            break

    if result == "*":
        res = board.result()
        if res == "*":
            res = "1/2-1/2" # Hit move limit
        result = res

    engine_w.quit()
    engine_b.quit()
    
    # Determine Rista's score
    if result == "1/2-1/2":
        score = 0.5
    elif result == "1-0":
        score = 1.0 if rista_is_white else 0.0
    elif result == "0-1":
        score = 0.0 if rista_is_white else 1.0
    else:
        score = 0.0
        
    print(f"[Match {match_id}] Finished! Result: {result} (Rista scored {score}) - Total moves: {len(move_history)}")
    if match_id == 1:
        print(f"\n[PROOF] Toàn bộ các nước đi của Match 1:\n{' '.join(move_history)}\n")
    return (match_id, score, result, rista_is_white)

if __name__ == "__main__":
    print("Starting 1 concurrent match to prove the move list...")
    num_matches = 1
    total_score = 0
    results = []
    
    with concurrent.futures.ThreadPoolExecutor(max_workers=num_matches) as executor:
        futures = [executor.submit(play_match, i + 1) for i in range(num_matches)]
        for future in concurrent.futures.as_completed(futures):
            try:
                match_id, score, res, is_white = future.result()
                results.append((match_id, score, res, is_white))
                total_score += score
            except Exception as exc:
                print(f"Match generated an exception: {exc}")
                
    print("\n" + "="*40)
    print("MATCH RESULTS")
    print("="*40)
    for match_id, score, res, is_white in sorted(results):
        color = "White" if is_white else "Black"
        print(f"Match {match_id}: Rista as {color:<5} -> Result: {res:>7} | Score: {score}")
    print("-" * 40)
    print(f"Final Score: Rista {total_score} / {num_matches}")
    print("="*40)
