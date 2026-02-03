# Настройка аутентификации GitHub

GitHub больше не поддерживает пароли для Git операций. Есть два способа:

## Способ 1: Personal Access Token (PAT) - Проще

### Шаг 1: Создайте Personal Access Token

1. Откройте https://github.com/settings/tokens
2. Нажмите "Generate new token" → "Generate new token (classic)"
3. Назовите токен (например, "CryptoSpider")
4. Выберите срок действия (например, "90 days" или "No expiration")
5. Отметьте права доступа:
   - ✅ `repo` (полный доступ к репозиториям)
6. Нажмите "Generate token"
7. **ВАЖНО**: Скопируйте токен сразу! Он показывается только один раз!

### Шаг 2: Используйте токен вместо пароля

При следующем `git push`:
- Username: ваш GitHub username (AfinaDev)
- Password: вставьте токен (не ваш пароль!)

Или измените URL на:
```bash
git remote set-url origin https://ВАШ-ТОКЕН@github.com/AfinaDev/CryptoSpider.git
```

## Способ 2: SSH ключи (Рекомендуется для постоянной работы)

### Шаг 1: Проверьте, есть ли SSH ключ

```bash
ls -la ~/.ssh
```

Если видите `id_rsa` или `id_ed25519` - ключ есть.

### Шаг 2: Создайте SSH ключ (если нет)

```bash
ssh-keygen -t ed25519 -C "ваш-email@example.com"
```

Нажмите Enter для всех вопросов (или укажите пароль для ключа).

### Шаг 3: Добавьте ключ в GitHub

1. Скопируйте публичный ключ:
```bash
cat ~/.ssh/id_ed25519.pub
# или
cat ~/.ssh/id_rsa.pub
```

2. Откройте https://github.com/settings/keys
3. Нажмите "New SSH key"
4. Вставьте скопированный ключ
5. Нажмите "Add SSH key"

### Шаг 4: Измените remote на SSH

```bash
git remote set-url origin git@github.com:AfinaDev/CryptoSpider.git
```

### Шаг 5: Проверьте подключение

```bash
ssh -T git@github.com
```

Должно ответить: "Hi AfinaDev! You've successfully authenticated..."

## Способ 3: GitHub CLI (gh)

```bash
# Установите GitHub CLI
brew install gh

# Авторизуйтесь
gh auth login

# Затем просто используйте git как обычно
```

## Быстрое решение (PAT)

Самый быстрый способ прямо сейчас:

1. Создайте токен: https://github.com/settings/tokens
2. Выполните:
```bash
git remote set-url origin https://ВАШ-ТОКЕН@github.com/AfinaDev/CryptoSpider.git
git push -u origin main
```

Или используйте токен как пароль при обычном push.
