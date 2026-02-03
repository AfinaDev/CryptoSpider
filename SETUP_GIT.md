# Настройка Git и GitHub

## Шаг 1: Создайте репозиторий на GitHub

1. Откройте https://github.com
2. Нажмите "New repository" (зеленая кнопка или "+" в правом верхнем углу)
3. Назовите репозиторий (например, `CryptoSpider`)
4. **НЕ** добавляйте README, .gitignore или лицензию (они уже есть)
5. Нажмите "Create repository"

## Шаг 2: Скопируйте URL репозитория

После создания репозитория GitHub покажет URL. Он будет выглядеть так:
- `https://github.com/ваш-username/CryptoSpider.git` (HTTPS)
- или `git@github.com:ваш-username/CryptoSpider.git` (SSH)

## Шаг 3: Выполните команды в терминале

```bash
cd /Users/a1/Documents/Dev/С++/CryptoSpider

# Добавьте все файлы
git add .

# Сделайте первый коммит
git commit -m "Initial commit: BEP20 vanity address generator"

# Добавьте remote (замените URL на ваш!)
git remote add origin https://github.com/ваш-username/CryptoSpider.git

# Запушьте код
git push -u origin main
```

## Если у вас уже есть репозиторий на GitHub

Просто выполните:
```bash
git remote add origin https://github.com/ваш-username/CryptoSpider.git
git branch -M main
git push -u origin main
```

## Если используете SSH вместо HTTPS

Если у вас настроен SSH ключ для GitHub:
```bash
git remote add origin git@github.com:ваш-username/CryptoSpider.git
git push -u origin main
```

## Проверка

После успешного push проверьте:
```bash
git remote -v
```

Должно показать:
```
origin  https://github.com/ваш-username/CryptoSpider.git (fetch)
origin  https://github.com/ваш-username/CryptoSpider.git (push)
```
