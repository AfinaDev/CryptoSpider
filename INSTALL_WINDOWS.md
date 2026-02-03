# Установка OpenSSL на Windows

Для компиляции проекта на Windows необходимо установить OpenSSL. Вот несколько способов:

## Способ 1: Установка через Win32 OpenSSL (Рекомендуется)

1. Скачайте OpenSSL для Windows с официального сайта:
   - https://slproweb.com/products/Win32OpenSSL.html
   - Выберите версию для вашей системы (Win64 или Win32)
   - Рекомендуется скачать установщик (EXE), а не ZIP

2. Установите OpenSSL в стандартную директорию (например, `C:\OpenSSL-Win64`)

3. Установите переменную окружения `OPENSSL_ROOT_DIR`:
   - Откройте "Система" → "Дополнительные параметры системы" → "Переменные среды"
   - Добавьте новую переменную:
     - Имя: `OPENSSL_ROOT_DIR`
     - Значение: `C:\OpenSSL-Win64` (или путь, куда вы установили)

4. Перезапустите CLion/CMake

## Способ 2: Использование vcpkg

1. Установите vcpkg (если еще не установлен):
   ```powershell
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   ```

2. Установите OpenSSL:
   ```powershell
   .\vcpkg install openssl:x64-windows
   ```

3. В CLion укажите toolchain файл vcpkg:
   - Settings → Build, Execution, Deployment → CMake
   - В поле "CMake options" добавьте:
     ```
     -DCMAKE_TOOLCHAIN_FILE=[путь к vcpkg]/scripts/buildsystems/vcpkg.cmake
     ```

## Способ 3: Использование Chocolatey

1. Установите Chocolatey (если еще не установлен):
   ```powershell
   Set-ExecutionPolicy Bypass -Scope Process -Force
   [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
   iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
   ```

2. Установите OpenSSL:
   ```powershell
   choco install openssl
   ```

3. Установите переменную окружения `OPENSSL_ROOT_DIR` на путь установки OpenSSL

## Способ 4: Ручная установка

1. Скачайте ZIP архив OpenSSL с https://slproweb.com/products/Win32OpenSSL.html

2. Распакуйте в удобное место (например, `C:\OpenSSL-Win64`)

3. Установите переменную окружения `OPENSSL_ROOT_DIR` на путь к распакованной папке

4. Перезапустите CLion/CMake

## Проверка установки

После установки перезапустите CLion и попробуйте снова скомпилировать проект. CMake должен автоматически найти OpenSSL.

Если проблемы остаются, проверьте:
- Переменная окружения `OPENSSL_ROOT_DIR` установлена правильно
- В папке OpenSSL есть подпапки `include` и `lib`
- Перезапущен CLion после установки переменных окружения
