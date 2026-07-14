#include "book.h"
#include "movegen.h"
#include "book_keys.h"
#include <fstream>
#include <iostream>
#include <random>

namespace Book {
    struct Entry {
        uint64_t key;
        uint16_t move;
        uint16_t weight;
        uint32_t learn;
    };

    std::vector<Entry> entries;
    bool has_book = false;

    uint64_t RandomPiece[768];
    uint64_t RandomCastle[4];
    uint64_t RandomEnPassant[8];
    uint64_t RandomTurn;

    void init_hash() {
        for (int i = 0; i < 768; i++) RandomPiece[i] = PolyglotRandom[i];
        for (int i = 0; i < 4; i++) RandomCastle[i] = PolyglotRandom[768 + i];
        for (int i = 0; i < 8; i++) RandomEnPassant[i] = PolyglotRandom[768 + 4 + i];
        RandomTurn = PolyglotRandom[768 + 4 + 8];
    }

    uint64_t compute_polyglot_hash(const Board& board) {
        uint64_t hash = 0;
        for (int sq = 0; sq < 64; sq++) {
            Piece p = board.piece_on(static_cast<Square>(sq));
            if (p != EMPTY_PIECE) {
                int pc = 0;
                if (piece_type(p) == PAWN) pc = 0;
                else if (piece_type(p) == KNIGHT) pc = 1;
                else if (piece_type(p) == BISHOP) pc = 2;
                else if (piece_type(p) == ROOK) pc = 3;
                else if (piece_type(p) == QUEEN) pc = 4;
                else if (piece_type(p) == KING) pc = 5;
                
                if (piece_color(p) == WHITE) pc = pc * 2 + 1;
                else pc = pc * 2;
                
                hash ^= RandomPiece[64 * pc + sq];
            }
        }

        if (board.castle_rights & WK) hash ^= RandomCastle[0];
        if (board.castle_rights & WQ) hash ^= RandomCastle[1];
        if (board.castle_rights & BK) hash ^= RandomCastle[2];
        if (board.castle_rights & BQ) hash ^= RandomCastle[3];

        if (board.en_passant != NO_SQ) {
            // Only include the en-passant key if a pawn of the side to move
            // can actually capture en passant, matching Polyglot's convention
            // and the way MoveGen::generate_pawn_moves determines EP_CAPTURE.
            U64 our_pawns = board.pieces[(board.side_to_move == WHITE) ? W_PAWN : B_PAWN];
            U64 cap_left = (board.side_to_move == WHITE) ? ((our_pawns << 7) & 0x7F7F7F7F7F7F7F7FULL) : ((our_pawns >> 9) & 0x7F7F7F7F7F7F7F7FULL);
            U64 cap_right = (board.side_to_move == WHITE) ? ((our_pawns << 9) & 0xFEFEFEFEFEFEFEFEULL) : ((our_pawns >> 7) & 0xFEFEFEFEFEFEFEFEULL);
            U64 ep_bit = (1ULL << board.en_passant);
            if ((cap_left | cap_right) & ep_bit) {
                int ep_file = board.en_passant % 8;
                hash ^= RandomEnPassant[ep_file];
            }
        }

        if (board.side_to_move == WHITE) {
            hash ^= RandomTurn;
        }

        return hash;
    }

    void init(const std::string& path) {
        init_hash();
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            has_book = false;
            return;
        }

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        size_t num_entries = size / sizeof(Entry);
        entries.resize(num_entries);
        file.read(reinterpret_cast<char*>(entries.data()), size);
        has_book = true;
        
        // Endianness swap for Polyglot (Big Endian)
        for (auto& entry : entries) {
            entry.key = __builtin_bswap64(entry.key);
            entry.move = __builtin_bswap16(entry.move);
            entry.weight = __builtin_bswap16(entry.weight);
        }
    }

    uint16_t get_move(const Board& board) {
        if (!has_book) return 0;
        
        uint64_t hash = compute_polyglot_hash(board);
        
        // Binary search
        int l = 0, r = entries.size() - 1;
        int first_match = -1;
        while (l <= r) {
            int m = l + (r - l) / 2;
            if (entries[m].key < hash) l = m + 1;
            else if (entries[m].key > hash) r = m - 1;
            else {
                first_match = m;
                r = m - 1; // find the first one
            }
        }
        
        if (first_match == -1) return 0;
        
        std::vector<uint16_t> possible_moves;
        std::vector<int> weights;
        int total_weight = 0;
        
        for (size_t i = first_match; i < entries.size(); i++) {
            if (entries[i].key != hash) break;
            
            uint16_t poly_move = entries[i].move;
            int to_file = poly_move & 7;
            int to_row = (poly_move >> 3) & 7;
            int from_file = (poly_move >> 6) & 7;
            int from_row = (poly_move >> 9) & 7;
            int promo = (poly_move >> 12) & 7;
            
            Square from = static_cast<Square>(from_row * 8 + from_file);
            Square to = static_cast<Square>(to_row * 8 + to_file);

            // Polyglot encodes castling as "king captures own rook"
            // (e1h1/e1a1 for white, e8h8/e8a8 for black). Remap to this
            // engine's castling destination squares (e1g1/e1c1/e8g8/e8c8)
            // before matching against MoveGen's generated moves.
            if (board.piece_on(from) == W_KING || board.piece_on(from) == B_KING) {
                if (from == E1) {
                    if (to == H1) to = G1;
                    else if (to == A1) to = C1;
                } else if (from == E8) {
                    if (to == H8) to = G8;
                    else if (to == A8) to = C8;
                }
            }

            // Map promotion
            uint16_t flag = 0;
            if (promo != 0) {
                if (promo == 1) flag = N_PROMO;
                else if (promo == 2) flag = B_PROMO;
                else if (promo == 3) flag = R_PROMO;
                else if (promo == 4) flag = Q_PROMO;
            }
            
            MoveList list;
            MoveGen::generate_pseudo_legal(board, list, false);
            for (int j = 0; j < list.count; j++) {
                Move m = list.moves[j];
                if (m.from() == from && m.to() == to) {
                    if (promo != 0 && (m.flags() & 3) != (flag & 3)) continue;
                    
                    Board temp = board;
                    temp.make_move(m);
                    if (!temp.is_in_check((temp.side_to_move == WHITE) ? BLACK : WHITE)) {
                        possible_moves.push_back(m.move);
                        weights.push_back(entries[i].weight);
                        total_weight += entries[i].weight;
                    }
                }
            }
        }
        
        if (possible_moves.empty()) return 0;
        
        if (total_weight == 0) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, possible_moves.size() - 1);
            return possible_moves[dis(gen)];
        }
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, total_weight - 1);
        int r_val = dis(gen);
        
        int current_weight = 0;
        for (size_t i = 0; i < possible_moves.size(); i++) {
            current_weight += weights[i];
            if (r_val < current_weight) {
                return possible_moves[i];
            }
        }
        
        return possible_moves.back();
    }

    bool is_loaded() {
        return has_book;
    }
}
