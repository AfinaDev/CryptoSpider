#include <iostream>
#include <string>
#include <random>
#include <iomanip>
#include <sstream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <mutex>

#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

#include "keccak.h"

// Структура для хранения результата
struct Result {
    std::vector<uint8_t> privateKey;
    std::vector<uint8_t> addressBytes;
    bool found = false;
};

// Глобальные переменные для масок (оптимизированные)
struct Mask {
    uint8_t prefix[2];  // Байты для префикса
    bool prefixWildcard[2];
    bool prefixCaseSensitive[2];  // true если нужно учитывать регистр
    uint8_t suffix[6];  // Байты для суффикса (4 или 6 символов)
    bool suffixWildcard[6];
    bool suffixCaseSensitive[6];  // true если нужно учитывать регистр
    int suffixLength = 4;  // Длина суффикса (4 или 6)
    bool checkCase = false;  // true если нужно проверять регистр
} globalMask;

// Атомарные переменные для синхронизации
std::atomic<long long> totalAttempts(0);
std::atomic<bool> found(false);
Result foundResult;
std::mutex resultMutex;

// Быстрая генерация случайного приватного ключа (оптимизированная)
void generatePrivateKeyFast(std::vector<uint8_t>& privateKey, std::mt19937& gen) {
    // Генерируем по 4 байта за раз для большей скорости
    uint32_t* ptr = reinterpret_cast<uint32_t*>(privateKey.data());
    for (int i = 0; i < 8; i++) {
        ptr[i] = gen();
    }
}

// Оптимизированное получение публичного ключа (с переиспользованием контекста)
std::vector<uint8_t> getPublicKeyFast(const std::vector<uint8_t>& privateKey, 
                                      EC_GROUP* group, BN_CTX* ctx) {
    EC_POINT* pub_point = EC_POINT_new(group);
    BIGNUM* priv = BN_bin2bn(privateKey.data(), 32, nullptr);
    
    if (EC_POINT_mul(group, pub_point, priv, nullptr, nullptr, ctx) != 1) {
        EC_POINT_free(pub_point);
        BN_free(priv);
        return {};
    }
    
    size_t pubKeyLen = EC_POINT_point2oct(group, pub_point, 
                                          POINT_CONVERSION_UNCOMPRESSED, 
                                          nullptr, 0, ctx);
    std::vector<uint8_t> publicKey(pubKeyLen);
    EC_POINT_point2oct(group, pub_point, POINT_CONVERSION_UNCOMPRESSED,
                       publicKey.data(), pubKeyLen, ctx);
    
    EC_POINT_free(pub_point);
    BN_free(priv);
    
    return publicKey;
}

// Быстрое вычисление адреса без создания строки
std::vector<uint8_t> getAddressBytes(const std::vector<uint8_t>& publicKey) {
    // Публичный ключ в формате 0x04 + X + Y (65 байт)
    // Берем только X и Y (пропускаем первый байт 0x04)
    std::vector<uint8_t> pubKeyBytes(publicKey.begin() + 1, publicKey.end());
    
    // Вычисляем Keccak-256 хеш
    std::vector<uint8_t> hash = Keccak::keccak256(pubKeyBytes);
    
    // Берем последние 20 байт (адрес)
    return std::vector<uint8_t>(hash.end() - 20, hash.end());
}

// Lookup таблица для быстрого преобразования байта в hex символ
static const char hexChars[] = "0123456789abcdef";

// Forward declarations
std::string addressBytesToHex(const std::vector<uint8_t>& addressBytes);
std::string getAddressWithChecksum(const std::vector<uint8_t>& addressBytes);

// Конвертация байтов адреса в hex строку (без checksum, нижний регистр)
std::string addressBytesToHex(const std::vector<uint8_t>& addressBytes) {
    std::stringstream ss;
    ss << "0x";
    for (uint8_t byte : addressBytes) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return ss.str();
}

// Получение адреса с checksum для проверки маски (EIP-55)
std::string getAddressWithChecksum(const std::vector<uint8_t>& addressBytes) {
    // EIP-55: хешируем адрес как строку в нижнем регистре (без 0x)
    std::string addressLower;
    for (uint8_t byte : addressBytes) {
        addressLower += hexChars[(byte >> 4) & 0x0F];
        addressLower += hexChars[byte & 0x0F];
    }
    
    // Вычисляем Keccak-256 хеш строки адреса
    std::vector<uint8_t> addressStrBytes(addressLower.begin(), addressLower.end());
    std::vector<uint8_t> hash = Keccak::keccak256(addressStrBytes);
    
    // Создаем адрес с checksum
    // Для каждого символа адреса берем соответствующий nibble из хеша
    // Если nibble >= 8, символ должен быть заглавным
    std::string result = "0x";
    for (size_t i = 0; i < addressLower.length(); i++) {
        char c = addressLower[i];
        
        // Получаем соответствующий nibble из хеша
        size_t hashByteIndex = i / 2;
        bool isHighNibble = (i % 2 == 0);
        uint8_t hashNibble = isHighNibble ? 
            ((hash[hashByteIndex] >> 4) & 0x0F) : 
            (hash[hashByteIndex] & 0x0F);
        
        // Применяем checksum (EIP-55): если nibble хеша >= 8, символ должен быть заглавным
        if (hashNibble >= 8 && c >= 'a' && c <= 'f') {
            c = c - 'a' + 'A';
        }
        
        result += c;
    }
    
    return result;
}

// Быстрая проверка маски на байтах (с учетом регистра)
bool matchesMaskFast(const std::vector<uint8_t>& addressBytes) {
    // Если нужно проверять регистр, получаем адрес с checksum
    std::string addressStr;
    if (globalMask.checkCase) {
        addressStr = getAddressWithChecksum(addressBytes);
    } else {
        // Просто нижний регистр
        addressStr = addressBytesToHex(addressBytes);
    }
    
    std::string addr = addressStr.substr(2); // Убираем 0x
    
    // Проверяем префикс (первые 2 символа)
    if (!globalMask.prefixWildcard[0]) {
        char addrChar = addr[0];
        char maskChar = globalMask.prefix[0];
        
        if (globalMask.prefixCaseSensitive[0]) {
            // Учитываем регистр
            if (addrChar != maskChar) return false;
        } else {
            // Не учитываем регистр
            if (std::tolower(addrChar) != std::tolower(maskChar)) return false;
        }
    }
    
    if (!globalMask.prefixWildcard[1]) {
        char addrChar = addr[1];
        char maskChar = globalMask.prefix[1];
        
        if (globalMask.prefixCaseSensitive[1]) {
            if (addrChar != maskChar) return false;
        } else {
            if (std::tolower(addrChar) != std::tolower(maskChar)) return false;
        }
    }
    
    // Проверяем суффикс (последние 4 или 6 символов)
    size_t suffixStart = addr.length() - globalMask.suffixLength;
    for (int i = 0; i < globalMask.suffixLength; i++) {
        if (!globalMask.suffixWildcard[i]) {
            char addrChar = addr[suffixStart + i];
            char maskChar = globalMask.suffix[i];
            
            if (globalMask.suffixCaseSensitive[i]) {
                if (addrChar != maskChar) return false;
            } else {
                if (std::tolower(addrChar) != std::tolower(maskChar)) return false;
            }
        }
    }
    
    return true;
}


// Конвертация приватного ключа в hex строку (всегда полный формат)
std::string privateKeyToHex(const std::vector<uint8_t>& privateKey) {
    std::stringstream ss;
    ss << "0x";
    ss << std::hex << std::setfill('0');
    for (uint8_t byte : privateKey) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

// Функция для проверки правильности вычисления адреса
std::string verifyAddressFromPrivateKey(const std::vector<uint8_t>& privateKey) {
    EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    BN_CTX* ctx = BN_CTX_new();
    BIGNUM* priv = BN_bin2bn(privateKey.data(), 32, nullptr);
    
    EC_POINT* pub_point = EC_POINT_new(group);
    
    if (EC_POINT_mul(group, pub_point, priv, nullptr, nullptr, ctx) != 1) {
        EC_POINT_free(pub_point);
        EC_GROUP_free(group);
        BN_free(priv);
        BN_CTX_free(ctx);
        return "";
    }
    
    size_t pubKeyLen = EC_POINT_point2oct(group, pub_point, 
                                          POINT_CONVERSION_UNCOMPRESSED, 
                                          nullptr, 0, ctx);
    std::vector<uint8_t> publicKey(pubKeyLen);
    EC_POINT_point2oct(group, pub_point, POINT_CONVERSION_UNCOMPRESSED,
                       publicKey.data(), pubKeyLen, ctx);
    
    // Вычисляем адрес
    std::vector<uint8_t> addressBytes = getAddressBytes(publicKey);
    std::string address = addressBytesToHex(addressBytes);
    
    EC_POINT_free(pub_point);
    EC_GROUP_free(group);
    BN_free(priv);
    BN_CTX_free(ctx);
    
    return address;
}

// Функция рабочего потока
void workerThread(int threadId, int numThreads) {
    // Создаем свой генератор случайных чисел для каждого потока
    std::random_device rd;
    std::mt19937 gen(rd() + threadId);
    
    // Создаем криптографические контексты для переиспользования
    EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    BN_CTX* ctx = BN_CTX_new();
    
    std::vector<uint8_t> privateKey(32);
    long long localAttempts = 0;
    const long long reportInterval = 50000; // Реже обновляем счетчик
    
    while (!found.load()) {
        localAttempts++;
        
        // Обновляем глобальный счетчик периодически
        if (localAttempts % reportInterval == 0) {
            totalAttempts.fetch_add(reportInterval);
            localAttempts = 0;
        }
        
        // Генерируем приватный ключ
        generatePrivateKeyFast(privateKey, gen);
        
        // Получаем публичный ключ
        std::vector<uint8_t> publicKey = getPublicKeyFast(privateKey, group, ctx);
        
        if (publicKey.empty()) {
            continue;
        }
        
        // Вычисляем адрес
        std::vector<uint8_t> addressBytes = getAddressBytes(publicKey);
        
        // Проверяем маску
        if (matchesMaskFast(addressBytes)) {
            // Нашли подходящий адрес!
            std::lock_guard<std::mutex> lock(resultMutex);
            if (!found.load()) {
                foundResult.privateKey = privateKey;
                foundResult.addressBytes = addressBytes;
                foundResult.found = true;
                found.store(true);
            }
            break;
        }
    }
    
    // Обновляем финальный счетчик
    totalAttempts.fetch_add(localAttempts);
    
    // Освобождаем ресурсы
    EC_GROUP_free(group);
    BN_CTX_free(ctx);
}

int main() {
    std::cout << "=== Генератор Vanity Адресов BEP20 (Оптимизированная версия) ===" << std::endl;
    std::cout << "Введите первые 2 символа после 0x (используйте ? для любого символа): ";
    std::string prefixMask;
    std::cin >> prefixMask;
    
    std::cout << "Введите последние 4 или 6 символов (используйте ? для любого символа): ";
    std::string suffixMask;
    std::cin >> suffixMask;
    
    // Валидация масок
    if (prefixMask.length() != 2) {
        std::cerr << "Ошибка: префикс должен содержать ровно 2 символа" << std::endl;
        return 1;
    }
    
    if (suffixMask.length() != 4 && suffixMask.length() != 6) {
        std::cerr << "Ошибка: суффикс должен содержать 4 или 6 символов" << std::endl;
        return 1;
    }
    
    // Проверяем, что символы валидны (0-9, a-f, A-F, ?)
    auto isValidHexChar = [](char c) {
        return (c >= '0' && c <= '9') || 
               (c >= 'a' && c <= 'f') || 
               (c >= 'A' && c <= 'F') || 
               c == '?';
    };
    
    for (char c : prefixMask) {
        if (!isValidHexChar(c)) {
            std::cerr << "Ошибка: недопустимый символ в префиксе: " << c << std::endl;
            return 1;
        }
    }
    
    for (char c : suffixMask) {
        if (!isValidHexChar(c)) {
            std::cerr << "Ошибка: недопустимый символ в суффиксе: " << c << std::endl;
            return 1;
        }
    }
    
    // Сохраняем оригинальные маски для проверки регистра
    std::string prefixMaskOriginal = prefixMask;
    std::string suffixMaskOriginal = suffixMask;
    
    // Проверяем, есть ли заглавные буквы в маске (значит нужно учитывать регистр)
    bool hasUpperCase = false;
    for (char c : prefixMask) {
        if (c >= 'A' && c <= 'F') {
            hasUpperCase = true;
            break;
        }
    }
    if (!hasUpperCase) {
        for (char c : suffixMask) {
            if (c >= 'A' && c <= 'F') {
                hasUpperCase = true;
                break;
            }
        }
    }
    
    globalMask.checkCase = hasUpperCase;
    globalMask.suffixLength = suffixMaskOriginal.length();
    
    // Заполняем структуру маски
    for (int i = 0; i < 2; i++) {
        if (prefixMaskOriginal[i] == '?') {
            globalMask.prefixWildcard[i] = true;
            globalMask.prefix[i] = 0;
            globalMask.prefixCaseSensitive[i] = false;
        } else {
            globalMask.prefixWildcard[i] = false;
            globalMask.prefix[i] = prefixMaskOriginal[i];
            // Если символ заглавный, значит нужно учитывать регистр
            globalMask.prefixCaseSensitive[i] = (prefixMaskOriginal[i] >= 'A' && prefixMaskOriginal[i] <= 'F');
        }
    }
    
    for (int i = 0; i < globalMask.suffixLength; i++) {
        if (suffixMaskOriginal[i] == '?') {
            globalMask.suffixWildcard[i] = true;
            globalMask.suffix[i] = 0;
            globalMask.suffixCaseSensitive[i] = false;
        } else {
            globalMask.suffixWildcard[i] = false;
            globalMask.suffix[i] = suffixMaskOriginal[i];
            // Если символ заглавный, значит нужно учитывать регистр
            globalMask.suffixCaseSensitive[i] = (suffixMaskOriginal[i] >= 'A' && suffixMaskOriginal[i] <= 'F');
        }
    }
    
    // Определяем количество потоков
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4; // Fallback если не можем определить
    
    std::cout << "\nПоиск адреса с маской: " << prefixMaskOriginal << "..." << suffixMaskOriginal << std::endl;
    if (globalMask.checkCase) {
        std::cout << "Регистр учитывается (EIP-55 checksum)" << std::endl;
    }
    std::cout << "Используется потоков: " << numThreads << std::endl;
    std::cout << "Запуск генерации..." << std::endl;
    
    auto startTime = std::chrono::steady_clock::now();
    auto lastReportTime = startTime;
    
    // Запускаем рабочие потоки
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < numThreads; i++) {
        threads.emplace_back(workerThread, i, numThreads);
    }
    
    // Поток для отображения прогресса
    std::thread progressThread([&]() {
        while (!found.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            auto now = std::chrono::steady_clock::now();
            auto totalElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
            auto reportElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastReportTime).count();
            
            long long currentAttempts = totalAttempts.load();
            
            if (totalElapsed > 0 && reportElapsed > 0) {
                double totalSeconds = totalElapsed / 1000.0;
                double speed = currentAttempts / totalSeconds;
                
                std::cout << "\rПопыток: " << currentAttempts 
                         << " | Скорость: " << std::fixed << std::setprecision(0) << speed 
                         << " адр/сек | Время: " << std::setprecision(1) << totalSeconds << "с" << std::flush;
            }
        }
    });
    
    // Ждем завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }
    
    found.store(true); // Останавливаем поток прогресса
    progressThread.join();
    
    auto elapsed = std::chrono::steady_clock::now() - startTime;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    
    if (foundResult.found) {
        // Проверяем правильность вычисления адреса
        std::string verifiedAddress = verifyAddressFromPrivateKey(foundResult.privateKey);
        std::string computedAddress = addressBytesToHex(foundResult.addressBytes);
        
        std::cout << "\n\n✓ Адрес найден!" << std::endl;
        
        // Выводим адрес с checksum, если нужно
        std::string addressToShow = globalMask.checkCase ? 
            getAddressWithChecksum(foundResult.addressBytes) : 
            computedAddress;
        std::cout << "Адрес: " << addressToShow << std::endl;
        if (globalMask.checkCase) {
            std::cout << "Адрес (lowercase): " << computedAddress << std::endl;
        }
        std::cout << "Приватный ключ: " << privateKeyToHex(foundResult.privateKey) << std::endl;
        
        // Проверка правильности
        if (verifiedAddress == computedAddress) {
            std::cout << "✓ Проверка: адрес вычислен правильно" << std::endl;
        } else {
            std::cout << "⚠ ОШИБКА: адрес не совпадает с проверкой!" << std::endl;
            std::cout << "  Вычисленный: " << computedAddress << std::endl;
            std::cout << "  Проверенный: " << verifiedAddress << std::endl;
        }
        
        std::cout << "Попыток: " << totalAttempts.load() << std::endl;
        std::cout << "Время: " << seconds << " секунд" << std::endl;
        std::cout << "\n⚠ ВНИМАНИЕ: Сохраните приватный ключ в безопасном месте!" << std::endl;
        std::cout << "Никому не показывайте приватный ключ!" << std::endl;
    } else {
        std::cout << "\nОшибка: адрес не найден" << std::endl;
        return 1;
    }
    
    return 0;
}