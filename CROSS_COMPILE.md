# Кросс-компиляция для Windows на Mac

Есть несколько способов скомпилировать проект для Windows на Mac:

## Способ 1: Использование MinGW-w64 (Рекомендуется для CLion)

### Установка MinGW-w64 на Mac:

1. Установите через Homebrew:
```bash
brew install mingw-w64
```

2. В CLion настройте новый Toolchain:
   - Settings → Build, Execution, Deployment → Toolchains
   - Нажмите `+` и выберите "MinGW"
   - Укажите путь к компилятору:
     - C Compiler: `/opt/homebrew/bin/x86_64-w64-mingw32-gcc` (или `/usr/local/bin/x86_64-w64-mingw32-gcc`)
     - C++ Compiler: `/opt/homebrew/bin/x86_64-w64-mingw32-g++` (или `/usr/local/bin/x86_64-w64-mingw32-g++`)
   - Debugger: оставьте пустым или укажите `x86_64-w64-mingw32-gdb`

3. Создайте новый CMake профиль:
   - Settings → Build, Execution, Deployment → CMake
   - Нажмите `+` для нового профиля
   - Назовите его "Windows Release" или "Windows Debug"
   - Выберите созданный MinGW toolchain
   - В "CMake options" добавьте:
     ```
     -DCMAKE_SYSTEM_NAME=Windows
     -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc
     -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++
     ```

4. **Проблема с OpenSSL**: MinGW-w64 на Mac не включает OpenSSL. Нужно либо:
   - Скомпилировать OpenSSL для Windows самостоятельно (сложно)
   - Использовать предкомпилированные библиотеки OpenSSL для Windows
   - Использовать альтернативный способ (см. ниже)

## Способ 2: GitHub Actions (Самый простой)

Создайте файл `.github/workflows/build-windows.yml`:

```yaml
name: Build Windows

on:
  push:
    branches: [ main ]
  workflow_dispatch:

jobs:
  build:
    runs-on: windows-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install OpenSSL
      run: |
        choco install openssl -y
    
    - name: Configure CMake
      run: |
        cmake -B build -DCMAKE_BUILD_TYPE=Release
    
    - name: Build
      run: |
        cmake --build build --config Release
    
    - name: Upload artifact
      uses: actions/upload-artifact@v3
      with:
        name: CryptoSpider-Windows
        path: build/Release/CryptoSpider.exe
```

Затем:
1. Загрузите код на GitHub
2. GitHub Actions автоматически скомпилирует .exe
3. Скачайте готовый .exe из артефактов

## Способ 3: Использование Docker с Windows toolchain

Создайте Dockerfile для компиляции (более сложный способ).

## Способ 4: Компиляция на Windows машине (Самый надежный)

1. Скопируйте проект на Windows машину
2. Установите OpenSSL (см. INSTALL_WINDOWS.md)
3. Откройте проект в CLion на Windows
4. Скомпилируйте как обычно

## Способ 5: Использование WSL (Windows Subsystem for Linux)

Если у вас есть доступ к Windows с WSL:
1. Установите WSL на Windows
2. Установите MinGW-w64 в WSL
3. Скомпилируйте через WSL

## Рекомендация

**Для быстрого результата**: Используйте Способ 2 (GitHub Actions) - это самый простой и надежный способ получить .exe файл без необходимости настраивать сложные toolchains.

**Для разработки**: Используйте Способ 4 - компилируйте прямо на Windows машине.
