# Спецификация NovelMind Editor: 100% готовность

> **Канонический документ**, определяющий критерии полностью готового редактора NovelMind

**Версия**: 1.0
**Дата**: 2025-12-23
**Статус**: Активный

---

## Содержание

1. [Product Definition](#1-product-definition)
2. [User Flows (E2E)](#2-user-flows-e2e)
3. [Functional Requirements by Module](#3-functional-requirements-by-module)
4. [Non-functional Requirements](#4-non-functional-requirements)
5. [Architecture Contracts](#5-architecture-contracts)
6. [Data Model & Persistence](#6-data-model--persistence)
7. [Error Handling & Diagnostics](#7-error-handling--diagnostics)
8. [Definition of Done (100%)](#8-definition-of-done-100)
9. [Coding Standards & RAII Rules](#9-coding-standards--raii-rules)
10. [Roadmap Mapping](#10-roadmap-mapping)

---

## 1. Product Definition

### 1.1 Назначение редактора

NovelMind Editor — это профессиональный визуальный редактор для создания визуальных новелл, предоставляющий:

- **Визуальное редактирование сцен** (WYSIWYG)
- **Визуальный редактор сценария** (StoryGraph)
- **Редактор временной шкалы** (Timeline) для анимаций
- **Управление локализацией** и озвучкой
- **Play-In-Editor** для тестирования без сборки
- **Систему сборки** в готовые исполняемые файлы

### 1.2 Границы ответственности

#### Редактор ДЕЛАЕТ:

| Функция | Описание |
|---------|----------|
| Создание и управление проектами | Новые проекты, шаблоны, открытие существующих |
| Редактирование сцен | Размещение фонов, персонажей, UI элементов |
| Создание сценариев | Визуальный граф узлов (диалоги, выборы, условия) |
| Анимация | Timeline для keyframe-анимаций, Curve Editor |
| Локализация | Редактор строковых таблиц, экспорт/импорт |
| Озвучка | Voice Manager для привязки аудиофайлов |
| Тестирование | Play-In-Editor с отладкой и точками останова |
| Сборка | Компиляция в .nmres пакеты и исполняемые файлы |

#### Редактор НЕ ДЕЛАЕТ:

| Функция | Причина |
|---------|---------|
| Создание графических ассетов | Используйте внешние редакторы (Photoshop, GIMP) |
| Обработка аудио | Используйте внешние DAW (Audacity, FL Studio) |
| Моделирование 3D | NovelMind — 2D движок |
| Кодогенерация игровой логики | NM Script покрывает все сценарии |

### 1.3 Целевые пользователи

1. **Сценаристы** — создание сюжета без программирования
2. **Художники** — размещение визуальных элементов
3. **Звукорежиссёры** — синхронизация озвучки
4. **Инди-разработчики** — полный цикл создания VN
5. **Студии** — командная разработка

---

## 2. User Flows (E2E)

### 2.1 Create Project → Import Assets → Story → Scene → Play/Preview → Build/Export

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   Создать   │ →  │   Импорт    │ →  │  Создать    │ →  │  Настроить  │
│   Проект    │    │   Ассеты    │    │  StoryGraph │    │    Сцены    │
└─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘
       │                  │                  │                  │
       ▼                  ▼                  ▼                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│                        Play-In-Editor                                │
│  • Тестирование диалогов                                            │
│  • Проверка переходов                                               │
│  • Отладка переменных                                               │
└─────────────────────────────────────────────────────────────────────┘
       │
       ▼
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   Сборка    │ →  │  Тестирование│ →  │   Релиз    │
│   Проекта   │    │   Билда     │    │            │
└─────────────┘    └─────────────┘    └─────────────┘
```

### 2.2 Voice Workflow

```
1. Экспорт таблицы озвучки → Voice Manager → Export (CSV/Excel)
2. Запись озвучки → Внешняя студия/Актёры
3. Импорт файлов → Assets/Voice/
4. Автопривязка → Voice Manager → Auto-link по паттерну имён
5. Ручная привязка → Для нестандартных имён
6. Превью → Прослушивание прямо в редакторе
7. Тестирование → Play-In-Editor с озвучкой
```

### 2.3 Localization Workflow

```
1. Создание исходного языка → Localization Panel → Add Language (en)
2. Написание текстов → StoryGraph → Dialogue Nodes
3. Добавление языков → Localization Panel → Add Language (ru, ja)
4. Экспорт для перевода → Export → CSV/PO/XLIFF
5. Импорт переводов → Import → CSV/PO/XLIFF
6. Проверка → Diagnostics Panel → Missing Translations
7. Тестирование → Play-In-Editor → Switch Language
```

### 2.4 Animation Workflow

```
1. Выбор объекта → Scene View → Click on object
2. Создание трека → Timeline Panel → Add Track
3. Добавление keyframes → Timeline → Click to add keyframe
4. Настройка кривых → Curve Editor → Adjust easing
5. Превью → Timeline → Play button
6. Синхронизация → Timeline ↔ Play Mode sync
```

### 2.5 Diagnostics Workflow

```
1. Автопроверка → Diagnostics Panel (обновляется автоматически)
2. Категории ошибок:
   - Missing resources (отсутствующие файлы)
   - Broken node connections (разорванные связи)
   - Missing translations (отсутствующие переводы)
   - Unreachable nodes (недостижимые узлы)
   - Cycles (циклы в графе)
3. Навигация → Click на ошибку → Переход к источнику
4. Исправление → Fix в соответствующей панели
5. Повторная проверка → Автоматически после изменений
```

---

## 3. Functional Requirements by Module

### 3.1 Main Window & Docking System

| ID | Требование | Приоритет | Статус |
|----|------------|-----------|--------|
| MW-01 | Главное окно с системой докинга | MUST | ✅ |
| MW-02 | Перемещение панелей (лево/право/верх/низ) | MUST | ✅ |
| MW-03 | Сохранение/загрузка макета | MUST | ✅ |
| MW-04 | Сброс к макету по умолчанию | MUST | ✅ |
| MW-05 | Система вкладок внутри панелей | MUST | ✅ |
| MW-06 | Тёмная тема по умолчанию | MUST | ✅ |
| MW-07 | Поддержка High-DPI | MUST | ✅ |

### 3.2 Scene View Panel

| ID | Требование | Приоритет | Статус |
|----|------------|-----------|--------|
| SV-01 | Рендеринг сцены (QGraphicsView) | MUST | ✅ |
| SV-02 | Панорамирование (средняя кнопка мыши) | MUST | ✅ |
| SV-03 | Зум (колесо прокрутки) | MUST | ✅ |
| SV-04 | Сетка с переключением | SHOULD | ✅ |
| SV-05 | Выбор объектов кликом | MUST | ✅ |
| SV-06 | Гизмо трансформации (Move/Rotate/Scale) | MUST | ✅ |
| SV-07 | Подсветка выбранного объекта | MUST | ✅ |
| SV-08 | Перетаскивание из Asset Browser | SHOULD | ⚠️ Planned |
| SV-09 | Подсветка слоёв | MAY | ❌ Planned |

### 3.3 StoryGraph Panel

| ID | Требование | Приоритет | Статус |
|----|------------|-----------|--------|
| SG-01 | Холст визуального редактирования | MUST | ✅ |
| SG-02 | Панорамирование и зум | MUST | ✅ |
| SG-03 | Создание узлов (палитра) | MUST | ✅ |
| SG-04 | Перетаскивание узлов | MUST | ✅ |
| SG-05 | Соединение узлов (Ctrl+Drag) | MUST | ✅ |
| SG-06 | Удаление узлов/соединений (Delete) | MUST | ✅ |
| SG-07 | Типы узлов (Entry, Dialogue, Choice, Scene, Label, Script) | MUST | ✅ |
| SG-08 | Индикатор текущего узла (Play mode) | MUST | ✅ |
| SG-09 | Точки останова (breakpoints) | MUST | ✅ |
| SG-10 | Мини-карта | MAY | ❌ Planned |
| SG-11 | Валидация ошибок (циклы, missing outputs) | SHOULD | ❌ Planned |
| SG-12 | Изменение размера узлов | MAY | ❌ Planned |

### 3.4 Timeline Panel

| ID | Требование | Приоритет | Статус |
|----|------------|-----------|--------|
| TL-01 | Отображение треков анимации | MUST | ✅ |
| TL-02 | Добавление/удаление треков | MUST | ✅ |
| TL-03 | Отображение keyframes | MUST | ✅ |
| TL-04 | Линейка с делениями | MUST | ✅ |
| TL-05 | Зум временной шкалы | MUST | ✅ |
| TL-06 | Перетаскивание keyframes | SHOULD | ⚠️ Partial |
| TL-07 | Привязка к сетке | MAY | ❌ Planned |
| TL-08 | Выбор кривой смягчения | SHOULD | ❌ Planned |
| TL-09 | Синхронизация с Play Mode | SHOULD | ❌ Planned |

### 3.5 Inspector Panel

| ID | Требование | Приоритет | Статус |
|----|------------|-----------|--------|
| IN-01 | Категории свойств | MUST | ✅ |
| IN-02 | Сворачиваемые группы | MUST | ✅ |
| IN-03 | Редактирование числовых полей | MUST | ✅ |
| IN-04 | Редактирование текстовых полей | MUST | ✅ |
| IN-05 | Выпадающий список (enum) | MUST | ✅ |
| IN-06 | Выбор цвета | MUST | ✅ |
| IN-07 | Выбор ассета | SHOULD | ✅ |
| IN-08 | Мгновенное применение | MUST | ✅ |
| IN-09 | Интеграция с Undo/Redo | SHOULD | ⚠️ Framework ready |
| IN-10 | Редактирование CurveRef | MAY | ❌ Planned |

### 3.6 Asset Browser Panel

| ID | Требование | Приоритет | Статус |
|----|------------|-----------|--------|
| AB-01 | Дерево папок | MUST | ✅ |
| AB-02 | Поиск по имени | MUST | ✅ |
| AB-03 | Фильтр по типу | MUST | ✅ |
| AB-04 | Grid/List view | SHOULD | ❌ Planned |
| AB-05 | Миниатюры изображений | SHOULD | ❌ Planned |
| AB-06 | Предпросмотр аудио | MAY | ❌ Planned |
| AB-07 | Метаданные ассетов | MAY | ❌ Planned |
| AB-08 | Контекстное меню (Rename/Delete) | SHOULD | ❌ Planned |
| AB-09 | Drag-and-drop на панели | SHOULD | ❌ Planned |

### 3.7 Curve Editor Panel

| ID | Требование | Приоритет | Статус |
|----|------------|-----------|--------|
| CE-01 | Отображение кривой | MUST | ✅ |
| CE-02 | Точки кривой | MUST | ✅ |
| CE-03 | Типы интерполяции | MUST | ✅ |
| CE-04 | Визуализация сетки | MUST | ✅ |
| CE-05 | Перетаскивание точек | MUST | ✅ |
| CE-06 | Библиотека пресетов | MAY | ✅ |

### 3.8 Hierarchy Panel

| ID | Требование | Приоритет | Статус |
|----|------------|-----------|--------|
| HP-01 | Дерево объектов сцены | MUST | ✅ |
| HP-02 | Множественный выбор | SHOULD | ❌ Planned |
| HP-03 | Drag-and-drop для reparenting | SHOULD | ❌ Planned |
| HP-04 | Контекстное меню | SHOULD | ❌ Planned |
| HP-05 | Синхронизация с Scene View | MUST | ⚠️ Partial |

### 3.9 Voice Manager Panel

| ID | Требование | Приоритет | Статус |
|----|------------|-----------|--------|
| VM-01 | Таблица строк озвучки | MUST | ✅ |
| VM-02 | Воспроизведение аудио | MUST | ✅ |
| VM-03 | Индикаторы статуса | SHOULD | ❌ Planned |
| VM-04 | Автопривязка по имени | SHOULD | ❌ Planned |
| VM-05 | Ручная привязка | SHOULD | ❌ Planned |
| VM-06 | Поиск строк | SHOULD | ❌ Planned |
| VM-07 | Экспорт/импорт таблиц | MAY | ❌ Planned |
| VM-08 | Навигация к StoryGraph | MAY | ❌ Planned |

### 3.10 Localization Panel

| ID | Требование | Приоритет | Статус |
|----|------------|-----------|--------|
| LP-01 | Таблица строк | MUST | ✅ |
| LP-02 | Редактирование переводов | MUST | ✅ |
| LP-03 | Поддержка нескольких языков | MUST | ✅ |
| LP-04 | Импорт/экспорт (CSV/JSON/PO/XLIFF) | MUST | ✅ |
| LP-05 | Поиск и фильтры | SHOULD | ❌ Planned |
| LP-06 | Подсветка отсутствующих переводов | SHOULD | ❌ Planned |
| LP-07 | Навигация к месту использования | MAY | ❌ Planned |

### 3.11 Console Panel

| ID | Требование | Приоритет | Статус |
|----|------------|-----------|--------|
| CP-01 | Вывод логов | MUST | ✅ |
| CP-02 | Сортировка по времени | MUST | ✅ |
| CP-03 | Фильтр по уровню | MUST | ✅ |
| CP-04 | Автопрокрутка | MUST | ✅ |
| CP-05 | Копирование в буфер | MUST | ✅ |
| CP-06 | Очистка консоли | MUST | ✅ |

### 3.12 Debug Overlay Panel

| ID | Требование | Приоритет | Статус |
|----|------------|-----------|--------|
| DO-01 | Переменные VM | MUST | ✅ |
| DO-02 | Текущая инструкция | MUST | ✅ |
| DO-03 | Стек вызовов | MUST | ✅ |
| DO-04 | Активные анимации | SHOULD | ✅ |
| DO-05 | Аудиоканалы | SHOULD | ✅ |
| DO-06 | Информация о времени кадра | SHOULD | ✅ |
| DO-07 | Режимы отображения (Minimal/Extended) | MAY | ✅ |

### 3.13 Diagnostics Panel

| ID | Требование | Приоритет | Статус |
|----|------------|-----------|--------|
| DG-01 | Список ошибок и предупреждений | MUST | ✅ |
| DG-02 | Категории | MUST | ✅ |
| DG-03 | Подсветка серьёзности | MUST | ✅ |
| DG-04 | Навигация к источнику | SHOULD | ❌ Planned |
| DG-05 | Автообновление в Play mode | SHOULD | ❌ Planned |
| DG-06 | Quick fixes | MAY | ❌ Planned |

### 3.14 Build Settings Panel

| ID | Требование | Приоритет | Статус |
|----|------------|-----------|--------|
| BS-01 | Настройки платформы | MUST | ✅ |
| BS-02 | Пресеты сборки | MUST | ✅ |
| BS-03 | Настройки шифрования | MUST | ✅ |
| BS-04 | Настройки сжатия | MUST | ✅ |
| BS-05 | Предпросмотр размера сборки | MAY | ❌ Planned |
| BS-06 | Предупреждения о missing ресурсах | SHOULD | ❌ Planned |
| BS-07 | Выполнение и статус сборки | SHOULD | ❌ Planned |

### 3.15 Play-In-Editor System

| ID | Требование | Приоритет | Статус |
|----|------------|-----------|--------|
| PE-01 | Play/Pause/Stop/Step | MUST | ✅ |
| PE-02 | Горячие клавиши (F5/F6/F7/F10) | MUST | ✅ |
| PE-03 | Навигация к активному узлу | MUST | ✅ |
| PE-04 | Save/Load состояния (слоты + auto) | MUST | ✅ |
| PE-05 | Редактирование переменных при паузе | MUST | ✅ |
| PE-06 | Точки останова | MUST | ✅ |
| PE-07 | Синхронизация Timeline | SHOULD | ❌ Planned |
| PE-08 | Hot reload | MAY | ❌ Planned |

### 3.16 Undo/Redo System

| ID | Требование | Приоритет | Статус |
|----|------------|-----------|--------|
| UR-01 | Единый стек команд (QUndoStack) | MUST | ✅ |
| UR-02 | Интеграция со StoryGraph | SHOULD | ⚠️ Framework ready |
| UR-03 | Интеграция с Timeline | SHOULD | ⚠️ Framework ready |
| UR-04 | Интеграция с Inspector | SHOULD | ⚠️ Framework ready |
| UR-05 | Undo операций с ассетами | MAY | ❌ Planned |

### 3.17 Event Bus & Selection

| ID | Требование | Приоритет | Статус |
|----|------------|-----------|--------|
| EB-01 | Централизованная шина событий | MUST | ✅ |
| EB-02 | Подписки панелей | MUST | ✅ |
| EB-03 | SelectionChanged events | MUST | ✅ |
| EB-04 | Единый источник истины для выбора | MUST | ✅ |
| EB-05 | Множественный выбор | SHOULD | ⚠️ Framework ready |
| EB-06 | История выбора | MAY | ❌ Planned |

---

## 4. Non-functional Requirements

### 4.1 Performance Goals

| Метрика | Цель | Примечание |
|---------|------|------------|
| Время запуска редактора | < 3 сек | На SSD, без плагинов |
| FPS в Scene View (100 объектов) | ≥ 60 FPS | Без сложных эффектов |
| Отклик UI | < 100ms | Любое действие пользователя |
| Потребление памяти (idle) | < 500 MB | Пустой проект |
| Время компиляции проекта (средний) | < 30 сек | 1000 узлов, 100 ресурсов |

### 4.2 Stability

| Требование | Цель |
|------------|------|
| Crash-free sessions | > 99% |
| Auto-save recovery | 100% восстановление после краша |
| Undo/Redo надёжность | 100% корректность |
| Hot reload стабильность | Без утечек памяти |

### 4.3 UX Responsiveness

| Действие | Ожидание |
|----------|----------|
| Клик на элемент | Немедленный отклик |
| Drag-and-drop | Плавное перемещение |
| Ввод текста | Без задержки |
| Переключение панелей | < 50ms |
| Открытие меню | Немедленно |

---

## 5. Architecture Contracts

### 5.1 GUI ↔ engine_core: Источник истины

```
┌─────────────────────────────────────────────────────────────────────┐
│                         GUI Layer (Qt)                               │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │  Panels (SceneView, StoryGraph, Inspector, Timeline, etc.)  │    │
│  └─────────────────────────────────────────────────────────────┘    │
│                              │                                       │
│                    (signals/slots)                                   │
│                              ▼                                       │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │  Event Bus (QtEventBus) + Selection (QtSelectionManager)    │    │
│  └─────────────────────────────────────────────────────────────┘    │
│                              │                                       │
│                    (adapters)                                        │
│                              ▼                                       │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │                    Adapter Layer                             │    │
│  │  NMAnimationAdapter, Preview Protocol, etc.                  │    │
│  └─────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────┘
                               │
                     (C++ API calls)
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      engine_core Layer                               │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐│
│  │ SceneGraph  │  │  Scripting  │  │   Audio     │  │    VFS      ││
│  │ (source of  │  │  (VM, IR)   │  │  Manager    │  │             ││
│  │   truth)    │  │             │  │             │  │             ││
│  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘│
└─────────────────────────────────────────────────────────────────────┘
```

**Правила**:

1. **SceneGraph** (engine_core) является **source of truth** для состояния сцены
2. GUI панели **читают** состояние из engine_core через адаптеры
3. GUI панели **изменяют** состояние через **команды** (QUndoCommand)
4. Изменения в engine_core **уведомляют** GUI через Event Bus

### 5.2 Event Bus: Правила использования

```cpp
// Типы событий
enum class EditorEventType {
    SelectionChanged,    // Изменился выбор
    PropertyChanged,     // Изменилось свойство объекта
    GraphModified,       // Изменён StoryGraph
    ProjectLoaded,       // Загружен проект
    UndoRedoPerformed,   // Выполнен undo/redo
    PlayModeChanged,     // Изменился режим Play
    AssetImported,       // Импортирован ассет
    LocaleChanged        // Изменён язык
};
```

**Контракт публикации**:
- Публиковать событие ПОСЛЕ успешного изменения состояния
- Включать достаточно данных для обновления всех подписчиков
- НЕ публиковать события в циклах (batching)

**Контракт подписки**:
- Подписываться в конструкторе, отписываться в деструкторе
- Обработчики ДОЛЖНЫ быть быстрыми (< 10ms)
- НЕ модифицировать состояние в обработчике напрямую

### 5.3 Undo Manager: Правила использования

```cpp
// Базовые классы команд
class PropertyChangeCommand : public QUndoCommand;
class AddObjectCommand : public QUndoCommand;
class DeleteObjectCommand : public QUndoCommand;
class TransformObjectCommand : public QUndoCommand;
class CreateGraphNodeCommand : public QUndoCommand;
class DeleteGraphNodeCommand : public QUndoCommand;
class ConnectGraphNodesCommand : public QUndoCommand;
```

**Контракт**:
- ВСЕ изменения состояния ДОЛЖНЫ проходить через команды
- Команды ДОЛЖНЫ быть обратимыми (undo восстанавливает предыдущее состояние)
- Команды МОГУТ объединяться (mergeWith) для плавных операций
- Команды НЕ ДОЛЖНЫ иметь побочных эффектов вне своего scope

---

## 6. Data Model & Persistence

### 6.1 Структура данных проекта

```
Project/
├── project.json           # Метаданные проекта
├── Assets/                # Ресурсы (images, audio, fonts)
├── Scripts/               # NM Script исходники
├── Scenes/                # JSON-файлы сцен
│   ├── scene_id.json      # Данные сцены (слои, объекты)
│   └── scene_id.graph.json # StoryGraph узлы и связи
├── Localization/          # Таблицы строк
│   ├── en.json
│   └── ru.json
└── Build/                 # Результаты сборки (gitignore)
```

### 6.2 Формат сериализации

**Сцены** (`scenes/*.json`):
```json
{
  "id": "scene_id",
  "name": "Scene Name",
  "background": "backgrounds/bg.png",
  "music": "music/theme.ogg",
  "layers": [
    { "type": "background", "objects": [...] },
    { "type": "characters", "objects": [...] },
    { "type": "ui", "objects": [...] }
  ],
  "entryNode": "node_start"
}
```

**StoryGraph** (`scenes/*.graph.json`):
```json
{
  "sceneId": "scene_id",
  "nodes": [
    {
      "id": "node_id",
      "type": "dialogue|choice|condition|transition|...",
      "position": { "x": 100, "y": 200 },
      "data": { ... },
      "next": "next_node_id"
    }
  ]
}
```

### 6.3 Dirty State & Save

**Правила dirty state**:
- Проект помечается dirty при ЛЮБОМ изменении через команду
- Auto-save каждые 5 минут (настраиваемо)
- При закрытии — подтверждение несохранённых изменений
- QUndoStack::cleanChanged → обновление заголовка окна

**Формат auto-save**:
```
~/.novelmind/autosave/
├── project_hash_timestamp.json  # Состояние проекта
└── project_hash_timestamp.assets/  # Временные ассеты (если изменены)
```

---

## 7. Error Handling & Diagnostics

### 7.1 Принципы сообщений пользователю

| Уровень | Отображение | Пример |
|---------|-------------|--------|
| **Error** | Modal dialog + Console (красный) | "Failed to load texture: file not found" |
| **Warning** | Console (жёлтый) + Status bar | "Missing translation for key 'dialog.001'" |
| **Info** | Console (синий) | "Project saved successfully" |
| **Debug** | Console (серый), только в Debug mode | "Loading scene: prologue" |

### 7.2 Формат location

```
StoryGraph:node_id           # Ошибка в узле StoryGraph
Script:path/script.nms:42    # Ошибка в скрипте, строка 42
Asset:images/sprite.png      # Проблема с ассетом
Scene:scene_id:layer:obj_id  # Проблема с объектом сцены
Localization:en:key_name     # Отсутствующий перевод
```

### 7.3 Категории диагностики

```cpp
enum class DiagnosticCategory {
    Asset,          // Проблемы с ресурсами
    Script,         // Ошибки NM Script
    Graph,          // Проблемы StoryGraph
    Localization,   // Отсутствующие переводы
    Voice,          // Несвязанная озвучка
    Build           // Ошибки сборки
};
```

---

## 8. Definition of Done (100%)

### 8.1 Чеклист готовности редактора

#### Основной функционал
- [x] Создание и открытие проектов
- [x] Редактирование сцен (SceneView)
- [x] Создание сценария (StoryGraph)
- [x] Редактирование свойств (Inspector)
- [x] Просмотр ресурсов (Asset Browser)
- [x] Play-In-Editor с отладкой
- [ ] Полная интеграция Timeline с анимациями
- [ ] Сборка в исполняемые файлы

#### Продакшн-инструменты
- [x] Локализация (импорт/экспорт)
- [x] Voice Manager (базовый)
- [ ] Полный Voice Manager (автопривязка, экспорт)
- [x] Diagnostics (базовый)
- [ ] Полный Diagnostics (навигация, quick fixes)

#### Качество
- [x] Темная тема
- [x] Система горячих клавиш
- [x] Undo/Redo (базовый)
- [ ] Полный Undo/Redo для всех операций
- [ ] Auto-save и recovery
- [ ] Документация пользователя

### 8.2 Минимальный набор автоматических тестов

```
tests/
├── unit/
│   ├── test_event_bus.cpp       # Event Bus публикация/подписка
│   ├── test_selection.cpp       # Selection manager
│   ├── test_undo_commands.cpp   # Все типы QUndoCommand
│   └── test_serialization.cpp   # JSON сериализация проекта
├── integration/
│   ├── test_scene_edit.cpp      # SceneView → Inspector → SceneGraph
│   ├── test_storygraph_edit.cpp # Создание/редактирование узлов
│   └── test_play_mode.cpp       # Play → Pause → Step → Stop
└── e2e/
    └── test_full_workflow.cpp   # Создание проекта → Сборка
```

### 8.3 Manual Regression Checklist

1. **Проект**
   - [ ] Создать новый проект из шаблона
   - [ ] Открыть существующий проект
   - [ ] Сохранить проект (Ctrl+S)
   - [ ] Закрыть с несохранёнными изменениями (должен спросить)

2. **Scene View**
   - [ ] Панорамирование средней кнопкой
   - [ ] Зум колесом
   - [ ] Выбор объекта кликом
   - [ ] Перемещение объекта гизмо

3. **StoryGraph**
   - [ ] Создать узел из палитры
   - [ ] Соединить узлы (Ctrl+Drag)
   - [ ] Удалить узел (Delete)
   - [ ] Удалить соединение (Delete)

4. **Inspector**
   - [ ] Изменить числовое свойство
   - [ ] Изменить текстовое свойство
   - [ ] Выбрать цвет
   - [ ] Undo/Redo изменения

5. **Play Mode**
   - [ ] Play (F5)
   - [ ] Pause (F6)
   - [ ] Step (F10)
   - [ ] Stop (F7)
   - [ ] Breakpoint в StoryGraph
   - [ ] Редактирование переменной при паузе

6. **Локализация**
   - [ ] Добавить язык
   - [ ] Редактировать перевод
   - [ ] Экспорт в CSV
   - [ ] Импорт из CSV

---

## 9. Coding Standards & RAII Rules

### 9.1 QObject Parent Ownership

```cpp
// ПРАВИЛЬНО: QObject управляет временем жизни child
QWidget* parent = new QWidget();
QPushButton* button = new QPushButton(parent);  // parent владеет button

// НЕПРАВИЛЬНО: утечка памяти
QPushButton* button = new QPushButton();  // Кто удалит?
```

### 9.2 Smart Pointers для engine_core

```cpp
// engine_core объекты всегда через smart pointers
std::unique_ptr<scene::AnimationTimeline> timeline;
std::shared_ptr<Resource> resource;

// ЗАПРЕЩЕНО: raw pointers на engine_core объекты
scene::AnimationTimeline* timeline;  // НЕЛЬЗЯ
```

### 9.3 QGraphicsItem Lifetime

```cpp
// QGraphicsScene владеет items
QGraphicsScene* scene = new QGraphicsScene(parent);
QGraphicsItem* item = new QGraphicsRectItem();
scene->addItem(item);  // scene владеет item

// Удаление:
scene->removeItem(item);
delete item;  // ИЛИ
// item->setParentItem(nullptr); + delete
```

### 9.4 Запрет Raw Pointers в QVariant

```cpp
// ЗАПРЕЩЕНО: нестабильный pointer в QVariant
item->setData(0, QVariant::fromValue(rawPointer));

// ПРАВИЛЬНО: стабильный ID
item->setData(0, QVariant::fromValue(objectId));  // QString ID
item->setData(0, QVariant::fromValue(index));     // int index
```

### 9.5 Правила signals/slots

```cpp
// ЗАПРЕЩЕНО: многократные connect в цикле
for (auto& item : items) {
    connect(item, &Item::clicked, this, &Panel::onClicked);
}

// ПРАВИЛЬНО: connect один раз при создании
void createItem(Item* item) {
    connect(item, &Item::clicked, this, &Panel::onClicked);
}

// ИЛИ: disconnect перед повторным connect
disconnect(item, &Item::clicked, this, &Panel::onClicked);
connect(item, &Item::clicked, this, &Panel::onClicked);
```

### 9.6 Common Pitfalls

| Проблема | Решение |
|----------|---------|
| Многократные connect'ы | Disconnect перед connect или connect только при создании |
| Блокирующий event loop в UI | Использовать QTimer::singleShot или QThread |
| Несогласованность модели и view | Обновлять view через сигналы модели |
| Утечки памяти в QGraphicsItem | Использовать scene->removeItem + delete |
| Raw pointer на engine_core | Всегда std::unique_ptr или std::shared_ptr |

---

## 10. Roadmap Mapping

### Маппинг требований на Issues

| Требование | Issue | Компонент | Статус |
|------------|-------|-----------|--------|
| StoryGraph редактирование | Issue #1-3 | nm_story_graph_panel | ✅ Done |
| Timeline keyframes | Issue #4-6 | nm_timeline_panel | ⚠️ In Progress |
| Inspector свойства | Issue #7 | nm_inspector_panel | ✅ Done |
| SceneView объекты | Issue #8-10 | nm_scene_view_panel | ✅ Done |
| Play-In-Editor | Issue #11-13 | nm_play_mode_controller | ✅ Done |
| Asset Browser | Issue #14 | nm_asset_browser_panel | ⚠️ Partial |
| Voice Manager полный | Issue #15-17 | nm_voice_manager_panel | ❌ Planned |
| Локализация полная | Issue #18-19 | nm_localization_panel | ⚠️ Partial |
| Diagnostics навигация | Issue #20 | nm_diagnostics_panel | ❌ Planned |
| Build система | Issue #21-23 | nm_build_settings_panel | ❌ Planned |

### Текущее состояние (по IMPLEMENTATION_ROADMAP.md)

| Категория | Завершено |
|-----------|-----------|
| Основа (Фазы 0-1) | 95% |
| Основное редактирование (Фаза 2) | 85% |
| Продвинутые редакторы (Фаза 3) | 75% |
| Продакшн-инструменты (Фаза 4) | 75% |
| Play-In-Editor (Фаза 5.0) | 95% |

---

## Приложение A: Ссылки на код

### Ключевые файлы GUI

| Компонент | Файл |
|-----------|------|
| Main Window | `editor/src/qt/nm_main_window.cpp` |
| Event Bus | `editor/src/qt/qt_event_bus.cpp` |
| Selection | `editor/src/qt/qt_selection_manager.cpp` |
| Undo Manager | `editor/src/qt/nm_undo_manager.cpp` |
| Play Controller | `editor/src/qt/nm_play_mode_controller.cpp` |
| Scene View | `editor/src/qt/panels/nm_scene_view_panel.cpp` |
| Story Graph | `editor/src/qt/panels/nm_story_graph_panel.cpp` |
| Inspector | `editor/src/qt/panels/nm_inspector_panel.cpp` |
| Timeline | `editor/src/qt/panels/nm_timeline_panel.cpp` |

### Ключевые файлы engine_core

| Компонент | Файл |
|-----------|------|
| Scene Graph | `engine_core/src/scene/scene_graph.cpp` |
| Script Runtime | `engine_core/src/scripting/script_runtime.cpp` |
| Audio Manager | `engine_core/src/audio/audio_manager.cpp` |
| Localization | `engine_core/src/localization/localization_manager.cpp` |
| Save Manager | `engine_core/src/save/save_manager.cpp` |
| VFS | `engine_core/src/vfs/virtual_fs.cpp` |

---

*Документ является living document и обновляется по мере развития проекта.*
