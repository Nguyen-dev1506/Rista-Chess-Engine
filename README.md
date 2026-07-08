# 🐟 Rista Chess Engine - Sát Thủ Cờ Vua Lấy Cảm Hứng Từ Loài Cá Lóc!

Chào mừng bạn đến với **Rista**, một Chess Engine được viết hoàn toàn bằng **C++ siêu tốc độ**, kết hợp với một chiếc GUI xinh xắn bằng **Python Tkinter**. Nếu bạn đang tìm kiếm một con bot cờ vua có khả năng tính toán vượt trội, thích "đập" luôn cả các bot khác, thì bạn đến đúng chỗ rồi đấy!

## 🚀 Tính năng nổi bật

- **Kiến trúc Lõi C++ Tối Ưu**: Sử dụng mô hình Board Representation 120-square (Mailbox), Rista chạy nhanh như một con cá lóc đang săn mồi.
- **Sinh Nước Đi Thông Minh (Move Generation)**: Khả năng bắt quân trượt, nhập thành, phong cấp, và ăn tốt qua đường (En Passant) chuẩn không cần chỉnh.
- **Thuật Toán Tìm Kiếm Cốt Lõi**: Trang bị Negamax kết hợp Cắt tỉa Alpha-Beta Pruning, và dĩ nhiên không thể thiếu **Quiescence Search** để chống lại "Horizon Effect" (Mù chân trời) — căn bệnh nan y khiến các engine ngớ ngẩn đút quân vào mồm đối thủ!
- **Đánh Giá Thế Trận (Evaluation)**: Ma trận Piece-Square Tables (PST) tinh xảo được thiết kế riêng.
- **Giao Thức UCI Chuẩn Quốc Tế**: Tương thích hoàn toàn với Universal Chess Interface, dễ dàng cắm vào mọi GUI trên thị trường.

## 🎮 Giao Diện Python GUI Đa Năng

Giao diện `rista_gui.py` không chỉ để bạn vào "ăn hành" với Rista (chế độ **Play as White vs Rista**), mà còn có một đấu trường La Mã thu nhỏ mang tên **"Rista vs Sunfish"**!

### Câu chuyện về trận đại chiến: Rista 🥊 Sunfish
Chúng tôi đã mời một vị khách đặc biệt là `sunfish.py` (một chess engine khá nổi tiếng bằng Python) vào giao diện để "so găng". 
Nhưng để đi đến chiến thắng, Rista đã phải trải qua một quá trình tu luyện gian khổ:
1. Rista từng bị một **Bug rò rỉ điểm Material Score trí mạng** (Mỗi lần nhẩm tính nước Phong Cấp ảo, nó tự trừ của mình đi 100 điểm cho đến khi điểm âm vô cực). Điều này khiến Rista bị trầm cảm và tự nguyện dâng Tốt đi tự sát (ví dụ như nước đi ngớ ngẩn `2...c4?` trong khai cuộc Sicilian).
2. Sau một buổi chẩn đoán gắt gao, chúng tôi đã "chữa lành" cho Rista.
3. **Kết quả?** Khi vừa bước vào võ đài **Rista vs Sunfish**, Rista (bằng C++ tối ưu) đã tính trước tận Depth 6 chỉ trong chớp mắt và hạ nốc ao bé Cá Thái Dương (Sunfish) một cách không thương tiếc! 🏆

## 🛠 Hướng Dẫn Cài Đặt Và Sử Dụng

### 1. Biên dịch C++ Engine
Đầu tiên, hãy gọi hồn con cá lóc bằng cách biên dịch mã nguồn C++:
```bash
make clean && make
```
*(Nếu bạn không dùng Make, cứ gõ chay `g++ -O3 -std=c++11 -o rista main.cpp board.cpp movegen.cpp search.cpp uci.cpp`)*

### 2. Khởi chạy Giao Diện
Và giờ là lúc mở giao diện lên để tận hưởng:
```bash
python3 rista_gui.py
```
*(Chú ý: Không cần cài thêm thư viện rườm rà, `sunfish.py` cũng đã được chúng tôi "mở khóa" vòng lặp UCI ẩn bên trong rồi, không lo thiếu thư viện `tools.uci` đâu nhé!)*

## 📈 Tương Lai Của Rista
- Tích hợp Zobrist Hashing và Transposition Table (Để Rista không phải suy nghĩ lại những thứ nó đã biết).
- Mở rộng Opening Book (Sách Khai Cuộc) để đa dạng hóa phong cách đánh.
- Thêm Null Move Pruning để tăng cường tốc độ cắt tỉa ở các thế trận tĩnh.

---
**Rista** - Không chỉ là cờ vua, đó là nghệ thuật của sự tiến hóa từ lỗi lầm (và sự ưu việt của C++ trước Python)! 😉
