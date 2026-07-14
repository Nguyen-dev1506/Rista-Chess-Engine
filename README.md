# 🐟 Rista Chess Engine - Sát Thủ Cờ Vua Lấy Cảm Hứng Từ Loài Cá Lóc!

<div align="center">
  <img src="Thanh_tich_Rista/rista_logo.png" alt="Rista Logo" width="300"/>
</div>

Chào mừng bạn đến với **Rista**, một Chess Engine được viết hoàn toàn bằng **C++ siêu tốc độ**, kết hợp với một chiếc GUI xinh xắn bằng **Python Tkinter**. Nếu bạn đang tìm kiếm một con bot cờ vua có khả năng tính toán vượt trội, thích "đập" luôn cả các bot khác, thì bạn đến đúng chỗ rồi đấy!

---

## 🏆 Chiến tích của Rista (Đại chiến với Fruit)
Rista không chỉ là "chuyên gia múa Hậu" mà còn có khả năng tìm ra những nước đi Brilliant (!!) cực kỳ ảo diệu. Trong trận đấu lịch sử với Engine gạo cội **Fruit**, Rista (phiên bản 3.3) đã thể hiện đẳng cấp vượt trội:

- **Độ chính xác (Accuracy) khủng khiếp:** Luôn dao động từ 96% đến 97.5%, không chừa cho đối thủ một con đường sống.
- **Nước đi Brilliant (!!):** Rista liên tục tung ra những đòn phối hợp chiến thuật, hiến quân sắc lẹm khiến Fruit cũng phải "mù mắt".

Dưới đây là một số hình ảnh chứng minh sức mạnh của bé Cá Lóc nhà ta trên bàn cờ:

<table align="center">
  <tr>
    <td><img src="Thanh_tich_Rista/Accuracies.png" alt="Accuracy 1" width="400"/></td>
    <td><img src="Thanh_tich_Rista/Accuracies 2.png" alt="Accuracy 2" width="400"/></td>
  </tr>
  <tr>
    <td><img src="Thanh_tich_Rista/Brilliant.png" alt="Brilliant 1" width="400"/></td>
    <td><img src="Thanh_tich_Rista/Brilliant 2.png" alt="Brilliant 2" width="400"/></td>
  </tr>
</table>

---

## 🚀 Tính năng nổi bật của Engine

- **Kiến trúc Lõi C++ Bitboard**: Xử lý bằng hệ thống Bitboard 64-bit hiện đại (Magic Bitboards), Rista chạy nhanh và càn quét các nhánh cờ như một con cá lóc đang săn mồi.
- **Sinh Nước Đi Thông Minh (Move Generation)**: Khả năng bắt quân trượt, nhập thành, phong cấp, và ăn tốt qua đường (En Passant) chuẩn không cần chỉnh.
- **Tối Ưu Tìm Kiếm (Search & Pruning)**:
  - Trang bị thuật toán **Negamax** kết hợp Cắt tỉa **Alpha-Beta Pruning**.
  - **Quiescence Search**: Chống lại "Horizon Effect" (Mù chân trời) — căn bệnh nan y khiến các engine ngớ ngẩn đút quân vào mồm đối thủ!
  - **ProbCut & SEE Pruning**: Tự động đánh giá chớp nhoáng và mạnh tay chặt bỏ những nhánh cờ "hiến quân mù quáng" để tập trung vào các nhánh tinh hoa.
  - **Capture History**: Học hỏi từ các nước ăn quân lịch sử gây đột biến để tối ưu thứ tự xét nước đi (Move Ordering).
- **Transposition Table (Bộ Nhớ Băm)**: Tích hợp Zobrist Hashing giúp Rista lưu lại các thế trận đã tính toán qua, giảm tải hàng triệu nút dư thừa và đẩy nhanh độ sâu (Depth).
- **Sách Khai Cuộc (Opening Book)**: Tự động tra cứu Polyglot `.bin` để đưa ra các khai cuộc đa dạng, thoát khỏi sự rập khuôn máy móc.
- **Giao Thức UCI Chuẩn Quốc Tế**: Tương thích hoàn toàn với Universal Chess Interface, dễ dàng cắm vào mọi GUI trên thị trường.

---

## 🎮 Giao Diện Python GUI (Rista GUI) Đa Năng

Rista đi kèm với một giao diện `rista_gui.py` được thiết kế riêng. Không chỉ là một bàn cờ vô hồn, nó là một "sân vận động" đầy đủ tiện nghi:

- **Tương Tác Kéo-Thả (Drag & Drop) Mượt Mà:** Trải nghiệm người dùng chân thực, cầm quân cờ kéo đi cực mượt mà không độ trễ. Thiết kế thân thiện với màu sắc hài hòa.
- **Theo Dõi Tư Duy Rista (Engine Log):** Tích hợp cửa sổ Terminal Log ngay trong GUI, hiển thị trực tiếp dòng thời gian Rista đang suy nghĩ (Độ sâu Depth, Điểm số, Nodes per second, và đặc biệt là Principal Variation - nhánh cờ mà Rista tính toán sâu nhất).
- **Thư Viện Đồ Họa Cờ Vua:** Các quân cờ (pieces) dạng SVG siêu nét, hỗ trợ mọi độ phân giải.
- **Chế độ PvP / PvE Đa Dạng:** Cho phép bạn vào "ăn hành" trực tiếp với Rista (chế độ **Play as White vs Rista**), hoặc tổ chức các trận tử chiến giữa Rista và những Bot cờ khác (như Sunfish).

### Câu chuyện về trận đại chiến: Rista 🥊 Sunfish
Chúng tôi đã từng mời một vị khách đặc biệt là `sunfish.py` (một chess engine khá nổi tiếng bằng Python) vào giao diện để "so găng". 
Nhưng để đi đến chiến thắng, Rista đã phải trải qua một quá trình tu luyện gian khổ:
1. Rista từng bị một **Bug rò rỉ điểm Material Score trí mạng** (Mỗi lần nhẩm tính nước Phong Cấp ảo, nó tự trừ của mình đi 100 điểm cho đến khi điểm âm vô cực). Điều này khiến Rista bị trầm cảm và tự nguyện dâng Tốt đi tự sát (ví dụ như nước đi ngớ ngẩn `2...c4?` trong khai cuộc Sicilian).
2. Sau một buổi chẩn đoán gắt gao, chúng tôi đã "chữa lành" cho Rista.
3. **Kết quả?** Khi vừa bước vào võ đài, Rista (bằng C++ tối ưu) đã tính trước tận Depth 6 chỉ trong chớp mắt và hạ nốc ao bé Cá Thái Dương (Sunfish) một cách không thương tiếc! 🏆

---

## 🛠 Hướng Dẫn Cài Đặt Và Sử Dụng

### 1. Biên dịch C++ Engine
Đầu tiên, hãy gọi hồn con cá lóc bằng cách biên dịch mã nguồn C++:
```bash
make clean && make -j4
```
*(Yêu cầu trình biên dịch hỗ trợ C++20)*

### 2. Khởi chạy Giao Diện
Và giờ là lúc mở giao diện lên để tận hưởng:
```bash
python3 gui/rista_gui.py
```
*(Chú ý: Chỉ cần cài thêm một vài thư viện đồ họa cơ bản của Python, chạy mượt trên cả Windows, macOS và Linux)*

---

## 🧠 Lời Cảm Ơn (Acknowledgments)
Dự án Rista vinh dự được kế thừa và học hỏi từ những người khổng lồ trong giới mã nguồn mở:
- **Stockfish Team & Cộng Đồng**: Cảm ơn các bạn đã cung cấp công cụ huấn luyện [nnue-pytorch](https://github.com/official-stockfish/nnue-pytorch) và chia sẻ các kho dữ liệu (Dataset) khổng lồ trên HuggingFace. Những tài nguyên này là cốt lõi để Rista chuẩn bị bước vào kỷ nguyên Mạng Nơ-ron (NNUE).

---
**Rista** - Không chỉ là cờ vua, đó là nghệ thuật của sự tiến hóa từ lỗi lầm (và sự ưu việt của C++ trước Python)! 😉
