cmake_minimum_required(VERSION 3.10)
project(CryptoSpider)

set(CMAKE_CXX_STANDARD 20)

# Определяем целевую платформу
if(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR WIN32)
    set(IS_WINDOWS TRUE)
else()
    set(IS_WINDOWS FALSE)
endif()

# Найти OpenSSL
find_package(OpenSSL REQUIRED)

add_executable(CryptoSpider main.cpp keccak.h)

# Подключить OpenSSL
target_link_libraries(CryptoSpider OpenSSL::Crypto)
target_include_directories(CryptoSpider PRIVATE ${OPENSSL_INCLUDE_DIR})

# Настройки для Windows
if(IS_WINDOWS)
    # Статическая линковка для Windows (чтобы не нужны были DLL)
    set_target_properties(CryptoSpider PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()
