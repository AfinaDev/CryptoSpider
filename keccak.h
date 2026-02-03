// Правильная реализация Keccak-256 для Ethereum адресов
// Основано на стандарте Keccak

#ifndef KECCAK_H
#define KECCAK_H

#include <vector>
#include <cstdint>
#include <cstring>

class Keccak {
private:
    static constexpr int KECCAK_ROUNDS = 24;
    static constexpr uint64_t ROTL64(uint64_t x, int y) {
        return (x << y) | (x >> (64 - y));
    }
    
    static void keccakf(uint64_t st[25]) {
        const uint64_t keccakf_rndc[24] = {
            0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
            0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
            0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
            0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
            0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
            0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
            0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
            0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
        };
        
        const int keccakf_rotc[24] = {
            1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 2, 14,
            27, 41, 56, 8, 25, 43, 62, 18, 39, 61, 20, 44
        };
        
        const int keccakf_piln[24] = {
            10, 7, 11, 17, 18, 3, 5, 16, 8, 21, 24, 4,
            15, 23, 19, 13, 12, 2, 20, 14, 22, 9, 6, 1
        };
        
        uint64_t t, bc[5];
        int i, j, round;
        
        for (round = 0; round < KECCAK_ROUNDS; round++) {
            // Theta
            for (i = 0; i < 5; i++)
                bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];
            
            for (i = 0; i < 5; i++) {
                t = bc[(i + 4) % 5] ^ ROTL64(bc[(i + 1) % 5], 1);
                for (j = 0; j < 25; j += 5)
                    st[j + i] ^= t;
            }
            
            // Rho Pi
            t = st[1];
            for (i = 0; i < 24; i++) {
                j = keccakf_piln[i];
                bc[0] = st[j];
                st[j] = ROTL64(t, keccakf_rotc[i]);
                t = bc[0];
            }
            
            // Chi
            for (j = 0; j < 25; j += 5) {
                for (i = 0; i < 5; i++)
                    bc[i] = st[j + i];
                for (i = 0; i < 5; i++)
                    st[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
            }
            
            // Iota
            st[0] ^= keccakf_rndc[round];
        }
    }
    
public:
    static std::vector<uint8_t> keccak256(const std::vector<uint8_t>& input) {
        uint64_t st[25] = {0};
        uint8_t temp[144];
        int rsiz = 200 - 2 * 32; // 136 для Keccak-256
        int rsizw = rsiz / 8;
        size_t inputLen = input.size();
        size_t offset = 0;
        
        // Обрабатываем все полные блоки
        while (offset + rsiz <= inputLen) {
            // Копируем данные в temp
            memcpy(temp, input.data() + offset, rsiz);
            
            // Преобразуем в uint64_t и XOR с состоянием
            for (int i = 0; i < rsizw; i++) {
                uint64_t word = 0;
                for (int j = 0; j < 8; j++) {
                    word |= ((uint64_t)temp[i * 8 + j]) << (j * 8);
                }
                st[i] ^= word;
            }
            
            keccakf(st);
            offset += rsiz;
        }
        
        // Обрабатываем последний неполный блок
        int remaining = inputLen - offset;
        memset(temp, 0, rsiz);
        if (remaining > 0) {
            memcpy(temp, input.data() + offset, remaining);
        }
        
        // Keccak padding: добавляем 0x01 в конец данных, затем 0x80 в конец блока
        temp[remaining] = 0x01;
        temp[rsiz - 1] ^= 0x80;
        
        // Преобразуем в uint64_t (little-endian) и XOR с состоянием
        for (int i = 0; i < rsizw; i++) {
            uint64_t word = 0;
            for (int j = 0; j < 8; j++) {
                word |= ((uint64_t)temp[i * 8 + j]) << (j * 8);
            }
            st[i] ^= word;
        }
        
        keccakf(st);
        
        // Извлекаем результат (32 байта) в little-endian формате
        std::vector<uint8_t> output(32);
        for (int i = 0; i < 4; i++) {
            uint64_t word = st[i];
            for (int j = 0; j < 8; j++) {
                output[i * 8 + j] = (word >> (j * 8)) & 0xFF;
            }
        }
        
        return output;
    }
};

#endif // KECCAK_H