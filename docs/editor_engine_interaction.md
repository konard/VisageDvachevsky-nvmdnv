# Взаимодействие редактора и движка

Этот документ описывает протоколы обмена данными и форматы между визуальным редактором NovelMind и runtime-движком.

## Обзор

Редактор и движок обмениваются данными через чётко определённые форматы:

```
+----------------+                      +----------------+
|                |   Файлы проекта      |                |
|   Редактор     | ------------------> |   Компилятор   |
|                |   (JSON, NMScript)   |                |
+----------------+                      +----------------+
       |                                       |
       | Превью                                | Сборка
       v                                       v
+----------------+                      +----------------+
|   Встроенный   |                      |  Файлы пака    |
|    Runtime     |                      |   (.nmres)     |
+----------------+                      +----------------+
                                               |
                                               v
                                        +----------------+
                                        |    Runtime     |
                                        |     движок     |
                                        +----------------+
```

## Формат файлов проекта

### Манифест проекта (project.json)

```json
{
  "version": "1.0",
  "name": "My Visual Novel",
  "displayName": "My Visual Novel: Complete Edition",
  "author": "Studio Name",
  "description": "A compelling visual novel experience",
  "settings": {
    "resolution": {
      "width": 1920,
      "height": 1080
    },
    "aspectRatio": "16:9",
    "defaultLanguage": "en",
    "supportedLanguages": ["en", "ja", "ru"],
    "startScene": "prologue"
  },
  "build": {
    "outputName": "MyVisualNovel",
    "version": "1.0.0",
    "platforms": ["windows", "linux", "macos"],
    "encryptAssets": true,
    "compressAssets": true
  }
}
```

### Определение персонажа (characters/*.json)

```json
{
  "id": "hero",
  "name": "Hero",
  "displayName": {
    "en": "Alex",
    "ja": "アレックス",
    "ru": "Алекс"
  },
  "defaultColor": "#FFCC00",
  "sprites": {
    "idle": "sprites/hero/idle.png",
    "happy": "sprites/hero/happy.png",
    "sad": "sprites/hero/sad.png",
    "angry": "sprites/hero/angry.png"
  },
  "defaultSprite": "idle",
  "voiceProfile": "voice/hero"
}
```

### Определение сцены (scenes/*.json)

```json
{
  "id": "cafe_meeting",
  "name": "Cafe Meeting",
  "background": "backgrounds/cafe_interior.png",
  "music": "music/peaceful_afternoon.ogg",
  "layers": [
    {
      "type": "background",
      "objects": [
        {
          "id": "bg",
          "type": "sprite",
          "resource": "backgrounds/cafe_interior.png",
          "position": { "x": 0, "y": 0 },
          "anchor": { "x": 0, "y": 0 }
        }
      ]
    },
    {
      "type": "characters",
      "objects": []
    },
    {
      "type": "ui",
      "objects": [
        {
          "id": "dialogue_box",
          "type": "ui_panel",
          "template": "dialogue_default"
        }
      ]
    }
  ],
  "entryNode": "node_start",
  "variables": {
    "local_flag": false
  }
}
```

### Граф сюжета (scenes/*.graph.json)

```json
{
  "sceneId": "cafe_meeting",
  "nodes": [
    {
      "id": "node_start",
      "type": "dialogue",
      "character": "hero",
      "sprite": "idle",
      "text": {
        "en": "Welcome to the cafe!",
        "ja": "カフェへようこそ！"
      },
      "next": "node_choice"
    },
    {
      "id": "node_choice",
      "type": "choice",
      "prompt": {
        "en": "What would you like to order?",
        "ja": "何を注文しますか？"
      },
      "choices": [
        {
          "text": { "en": "Coffee", "ja": "コーヒー" },
          "next": "node_coffee"
        },
        {
          "text": { "en": "Tea", "ja": "紅茶" },
          "next": "node_tea"
        }
      ]
    },
    {
      "id": "node_coffee",
      "type": "dialogue",
      "character": "hero",
      "text": { "en": "Good choice!" },
      "actions": [
        { "type": "set_flag", "flag": "chose_coffee", "value": true }
      ],
      "next": "node_end"
    },
    {
      "id": "node_tea",
      "type": "dialogue",
      "character": "hero",
      "text": { "en": "Excellent taste!" },
      "next": "node_end"
    },
    {
      "id": "node_end",
      "type": "transition",
      "target": "next_scene"
    }
  ]
}
```

### Типы узлов

| Тип | Описание |
|------|-------------|
| `dialogue` | Персонаж говорит с текстом |
| `narration` | Текст рассказчика без персонажа |
| `choice` | Разветвление выбора игрока |
| `condition` | Условное разветвление |
| `transition` | Переход между сценами |
| `action` | Выполнение команд без текста |
| `wait` | Приостановка выполнения |

### Типы действий

| Тип | Параметры | Описание |
|------|------------|-------------|
| `show_character` | character, sprite, position | Показать спрайт персонажа |
| `hide_character` | character | Скрыть персонажа |
| `set_sprite` | character, sprite | Изменить спрайт персонажа |
| `move_character` | character, position, duration | Анимировать позицию персонажа |
| `set_flag` | flag, value | Установить логический флаг |
| `set_variable` | variable, value | Установить значение переменной |
| `play_sound` | sound | Воспроизвести звуковой эффект |
| `play_music` | music, fadeIn | Воспроизвести/сменить музыку |
| `stop_music` | fadeOut | Остановить музыку |
| `set_background` | background, transition | Сменить фон |
| `screen_effect` | effect, params | Применить эффект экрана |
| `wait` | duration | Ожидание в течение времени |
| `call_script` | script | Выполнить NM Script |

## Формат NM Script

### Базовый синтаксис

```
# Определение персонажей
character Hero(name="Alex", color="#FFCC00")
character Sidekick(name="Sam", color="#00CCFF")

# Объявление переменных
var relationship = 0
flag has_key = false

# Определение сцены
scene cafe_intro {
    # Команды
    background "cafe_interior"
    music "peaceful_afternoon" fade 2.0

    show Hero at center
    show Sidekick at right

    # Диалог
    Hero: "Welcome!"
    Sidekick: "Thanks for having us."

    # Выбор
    choice "What would you like?" {
        "Coffee" {
            Hero: "Good choice!"
            set has_coffee = true
        }
        "Tea" {
            Hero: "Excellent!"
        }
    }

    # Условие
    if has_key {
        Hero: "Let's go!"
        goto secret_room
    } else {
        Hero: "We need to find the key first."
    }
}
```

### Справка по командам

```
# Команды отображения
background <resource> [transition <type> <duration>]
show <character> [<sprite>] at <position> [with <animation>]
hide <character> [with <animation>]
sprite <character> <sprite>
move <character> to <position> [over <duration>]

# Аудио команды
music <resource> [fade <duration>] [loop]
sound <resource> [volume <level>]
stop music [fade <duration>]

# Текстовые команды
<character>: "<text>"
<character> (<sprite>): "<text>"
narrate: "<text>"

# Управление потоком
choice [<prompt>] { ... }
if <condition> { ... } [else { ... }]
goto <scene>
call <script>
return
wait <duration>

# Переменные
var <name> = <value>
flag <name> = <true|false>
set <name> = <value>

# Эффекты
fade <in|out> [<duration>] [<color>]
shake [<intensity>] [<duration>]
flash [<color>] [<duration>]
```

## Формат скомпилированного скрипта

NM Script компилируется в байткод для эффективного выполнения в runtime.

### Заголовок байткода

```
Смещение  Размер  Описание
0x00      4       Magic: "NMSC"
0x04      2       Версия
0x06      2       Флаги
0x08      4       Количество инструкций
0x0C      4       Размер пула констант
0x10      4       Размер таблицы строк
0x14      4       Размер таблицы символов
```

### Формат инструкции

```
Байт 0: Опкод (8 бит)
Байты 1-4: Операнд (32 бита, зависит от опкода)
```

### Пул констант

Содержит литералы, используемые в скрипте:
- Целочисленные константы
- Константы с плавающей точкой
- Ссылки на строки (смещение в таблице строк)

### Таблица строк

UTF-8 строки с нулевым окончанием для:
- Имён персонажей
- Текста диалогов
- Идентификаторов ресурсов

## Протокол превью

Редактор содержит встроенный runtime для превью в реальном времени. Связь использует простой командный протокол.

### Команды превью (Редактор -> Runtime)

```json
{"cmd": "load_scene", "scene": "cafe_meeting"}
{"cmd": "goto_node", "node": "node_choice"}
{"cmd": "set_variable", "name": "relationship", "value": 10}
{"cmd": "set_flag", "name": "has_key", "value": true}
{"cmd": "save_state", "slot": 1}
{"cmd": "load_state", "slot": 1}
{"cmd": "auto_save"}
{"cmd": "auto_load"}
{"cmd": "step"}
{"cmd": "pause"}
{"cmd": "resume"}
{"cmd": "reset"}
```

### События превью (Runtime -> Редактор)

```json
{"event": "scene_loaded", "scene": "cafe_meeting"}
{"event": "node_entered", "node": "node_start", "type": "dialogue"}
{"event": "dialogue", "character": "hero", "text": "Welcome!"}
{"event": "choice_presented", "choices": ["Coffee", "Tea"]}
{"event": "variable_changed", "name": "relationship", "value": 10}
{"event": "save_completed", "slot": 1}
{"event": "load_completed", "slot": 1}
{"event": "error", "message": "Resource not found: missing.png"}
```

## Конвейер ресурсов

### Поддерживаемые исходные форматы

| Тип | Форматы | Примечания |
|------|---------|-------|
| Изображения | PNG, JPG, WebP | PNG предпочтительнее для прозрачности |
| Аудио | OGG, WAV, MP3 | OGG предпочтительнее для музыки |
| Шрифты | TTF, OTF | |
| Скрипты | .nms | Исходный код NM Script |

### Обработка ресурсов

1. **Изображения**
   - Преобразовать в оптимальный формат
   - Генерировать mipmap-уровни (опционально)
   - Упаковать в текстурные атласы (спрайты)
   - Сохранить как есть (фоны)

2. **Аудио**
   - Преобразовать в OGG Vorbis
   - Нормализовать уровни громкости
   - Подготовить для потоковой передачи (музыка)

3. **Шрифты**
   - Растеризовать в растровые шрифты
   - Генерировать SDF для масштабируемого текста

4. **Скрипты**
   - Разобрать и проверить
   - Скомпилировать в байткод
   - Связать ссылки

## Данные локализации

### Формат таблицы строк (Localization/*.json)

```json
{
  "language": "en",
  "strings": {
    "dialogue.cafe_meeting.node_start": "Welcome to the cafe!",
    "dialogue.cafe_meeting.node_choice.prompt": "What would you like?",
    "choice.cafe_meeting.coffee": "Coffee",
    "choice.cafe_meeting.tea": "Tea",
    "ui.save": "Save",
    "ui.load": "Load",
    "ui.options": "Options"
  }
}
```

### Соглашение об идентификаторах строк

```
dialogue.<scene>.<node>         - Текст диалога
choice.<scene>.<choice_id>      - Текст выбора
narration.<scene>.<node>        - Текст повествования
ui.<element>                    - Текст UI
character.<id>.name             - Отображаемое имя персонажа
menu.<menu>.<item>              - Элементы меню
```

## Структура выходных файлов сборки

```
build/
├── windows/
│   ├── MyVisualNovel.exe       # Исполняемый файл
│   └── data.nmres              # Пакет ресурсов
├── linux/
│   ├── MyVisualNovel           # Исполняемый файл
│   └── data.nmres              # Пакет ресурсов
└── macos/
    └── MyVisualNovel.app/      # Пакет приложения
        └── Contents/
            ├── MacOS/
            │   └── MyVisualNovel
            └── Resources/
                └── data.nmres
```

## Режим отладки

В отладочных сборках движок может:

1. Загружать неупакованные ресурсы напрямую с диска
2. Горячо перезагружать изменённые ресурсы
3. Выполнять команды скриптов из консоли
4. Отображать отладочные оверлеи (FPS, память и т.д.)
5. Записывать детальные трассировки выполнения

### Команды отладочной консоли

```
reload scene              - Перезагрузить текущую сцену
reload script <name>      - Перезагрузить конкретный скрипт
goto <scene> [node]       - Перейти к сцене/узлу
set <var> <value>         - Установить переменную
flag <name> <true|false>  - Установить флаг
dump vars                 - Вывести все переменные
dump flags                - Вывести все флаги
screenshot                - Сохранить скриншот
profile start/stop        - Профилирование CPU
memory                    - Статистика памяти
```
