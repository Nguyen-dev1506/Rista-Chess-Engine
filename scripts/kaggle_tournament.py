import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'libs'))

import chess
import chess.engine
import chess.pgn
import concurrent.futures
import threading
import time
import os
import random
import logging

# Tắt log chuẩn của python-chess để tránh rác màn hình
logging.getLogger("chess.engine").setLevel(logging.WARNING)

# =====================================================================
# CẤU HÌNH GIẢI ĐẤU
# =====================================================================
# Chạy script này tại thư mục gốc của dự án (nơi chứa file rista)
ENGINE_1_PATH = "./rista"
ENGINE_2_PATH = "./opponents/Fruit-2.1-master/src/fruit"

# Thời gian suy nghĩ cho mỗi nước đi
# Có thể dùng Limit(time=1.0) hoặc Limit(depth=6)
LIMIT = chess.engine.Limit(time=1.0) 

TOTAL_GAMES = 100
CONCURRENCY = 4 # Số luồng chạy song song (4 matches cùng lúc)
PGN_OUTPUT = "kaggle_tournament_results.pgn"

# Danh sách một số khai cuộc ngẫu nhiên để các ván không bị trùng lặp
OPENINGS = [
    ["e2e4", "e7e5"], ["e2e4", "c7c5"], ["d2d4", "d7d5"], ["d2d4", "g8f6"],
    ["c2c4", "e7e5"], ["g1f3", "d7d5"], ["e2e4", "e7e6"], ["e2e4", "c7c6"],
    ["d2d4", "f7f5"], ["b2b3", "e7e5"], ["f2f4", "d7d5"], ["e2e4", "d7d6"],
    ["e2e4", "g7g6"], ["d2d4", "g8f6", "c2c4", "g7g6"],
    ["e2e4", "e7e5", "g1f3", "b8c6", "f1c4"],
    ["d2d4", "d7d5", "c2c4", "c7c6"]
]

# =====================================================================

results = {"Rista_Wins": 0, "Fruit_Wins": 0, "Draws": 0}
lock = threading.Lock()

def play_game(game_id):
    # Luân phiên Trắng / Đen xen kẽ
    if game_id % 2 == 0:
        white_path, white_name = ENGINE_1_PATH, "Rista"
        black_path, black_name = ENGINE_2_PATH, "Fruit 2.1"
    else:
        white_path, white_name = ENGINE_2_PATH, "Fruit 2.1"
        black_path, black_name = ENGINE_1_PATH, "Rista"

    try:
        # Khởi tạo engine
        engine_white = chess.engine.SimpleEngine.popen_uci(white_path)
        engine_black = chess.engine.SimpleEngine.popen_uci(black_path)
    except Exception as e:
        print(f"[Game {game_id}] Lỗi khởi động engine: {e} (Kiểm tra lại đường dẫn hoặc compile chưa)")
        return



    board = chess.Board()
    
    # Ép khai cuộc ngẫu nhiên
    opening = random.choice(OPENINGS)
    for uci_move in opening:
        board.push(chess.Move.from_uci(uci_move))

    # Khởi tạo ghi chép PGN
    game = chess.pgn.Game()
    game.headers["Event"] = f"Kaggle Rista vs Fruit Tournament"
    game.headers["Round"] = str(game_id)
    game.headers["White"] = white_name
    game.headers["Black"] = black_name
    
    node = game
    for uci_move in opening:
        node = node.add_variation(chess.Move.from_uci(uci_move))

    # Bắt đầu đánh
    while not board.is_game_over():
        try:
            if board.turn == chess.WHITE:
                result = engine_white.play(board, LIMIT)
            else:
                result = engine_black.play(board, LIMIT)
                
            board.push(result.move)
            node = node.add_variation(result.move)
        except Exception as e:
            print(f"[Game {game_id}] Lỗi trong lúc đánh: {e}")
            break

    engine_white.quit()
    engine_black.quit()

    # Phân tích kết quả
    outcome = board.outcome()
    winner = outcome.winner if outcome else None
    
    with lock:
        if winner == chess.WHITE:
            if white_name == "Rista":
                results["Rista_Wins"] += 1
            else:
                results["Fruit_Wins"] += 1
        elif winner == chess.BLACK:
            if black_name == "Rista":
                results["Rista_Wins"] += 1
            else:
                results["Fruit_Wins"] += 1
        else:
            results["Draws"] += 1
            
        print(f"[Game {game_id}] Kết thúc: {board.result()}. Tỉ số: Rista {results['Rista_Wins']} - {results['Fruit_Wins']} Fruit (Hòa {results['Draws']})")

    # Lưu PGN
    game.headers["Result"] = board.result()
    with lock:
        with open(PGN_OUTPUT, "a", encoding="utf-8") as f:
            f.write(str(game) + "\n\n")

if __name__ == "__main__":
    if not os.path.exists(ENGINE_1_PATH):
        print(f"Lỗi: Không tìm thấy file {ENGINE_1_PATH}. Bạn nhớ gõ 'make -j4' trước nhé!")
        exit(1)
        
    print(f"BẮT ĐẦU GIẢI ĐẤU {TOTAL_GAMES} VÁN TRÊN {CONCURRENCY} LUỒNG SONG SONG...")
    if os.path.exists(PGN_OUTPUT):
        os.remove(PGN_OUTPUT)
        
    start_time = time.time()
    
    # Chạy đa luồng
    with concurrent.futures.ThreadPoolExecutor(max_workers=CONCURRENCY) as executor:
        executor.map(play_game, range(1, TOTAL_GAMES + 1))
        
    end_time = time.time()
    print("="*50)
    print(f"GIẢI ĐẤU KẾT THÚC SAU {end_time - start_time:.2f} GIÂY")
    print(f"TỔNG KẾT: Rista Thắng {results['Rista_Wins']} | Fruit Thắng {results['Fruit_Wins']} | Hòa {results['Draws']}")
    print(f"File PGN đã được lưu tại: {PGN_OUTPUT}")
    print("="*50)
