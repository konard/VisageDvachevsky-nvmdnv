# Стандарты кодирования

Данный документ определяет стандарты кодирования и соглашения для проекта NovelMind.

## Языковой стандарт

- **Стандарт C++**: C++20 (с возможностями C++23, где это полезно)
- **Поддержка компиляторов**: GCC 11+, Clang 14+, MSVC 2022+

## Соглашения об именовании

### Общие правила

| Элемент | Стиль | Пример |
|---------|-------|---------|
| Классы/Структуры | PascalCase | `SceneManager`, `DialogueNode` |
| Функции/Методы | camelCase | `loadTexture()`, `getCurrentScene()` |
| Переменные | camelCase | `playerName`, `currentFrame` |
| Переменные-члены | префикс m_ + camelCase | `m_windowHandle`, `m_isRunning` |
| Константы | ALL_CAPS | `MAX_SPRITES`, `DEFAULT_WIDTH` |
| Перечисления | PascalCase | `enum class RenderMode` |
| Значения перечислений | PascalCase | `RenderMode::Immediate` |
| Пространства имен | PascalCase | `NovelMind::Core`, `NovelMind::VFS` |
| Макросы | ALL_CAPS с префиксом NOVELMIND_ | `NOVELMIND_ASSERT`, `NOVELMIND_DEBUG` |
| Параметры шаблонов | PascalCase | `template<typename ValueType>` |
| Имена файлов | snake_case | `scene_manager.hpp`, `vfs_interface.cpp` |

### Структура пространств имен

```cpp
namespace NovelMind {
    namespace Core { }      // Базовые утилиты, типы, result
    namespace Platform { }  // Абстракция платформы
    namespace VFS { }       // Виртуальная файловая система
    namespace Renderer { }  // 2D рендеринг
    namespace Scripting { } // Движок скриптов
    namespace Scene { }     // Управление сценами
    namespace Audio { }     // Аудиосистема
    namespace Input { }     // Обработка ввода
    namespace Save { }      // Система сохранения/загрузки
}
```

**Примечание**: Пространство имен верхнего уровня — `NovelMind`, а не `nm`. Это обеспечивает:
- Четкое, самодокументирующееся пространство имен, которое узнаваемо в любой кодовой базе
- Отсутствие конфликтов с другими библиотеками
- Профессиональный, отполированный вид

## Форматирование кода

### Отступы и пробелы

- **Отступы**: 4 пробела (без табуляции)
- **Длина строки**: Максимум 100 символов
- **Фигурные скобки**: Стиль Allman (открывающая скобка на новой строке)

```cpp
namespace NovelMind::Core
{

class Application
{
public:
    explicit Application(const Config& config);

    void run();
    void shutdown();

private:
    void processEvents();
    void update(float deltaTime);
    void render();

    bool m_isRunning;
    Config m_config;
};

} // namespace NovelMind::Core
```

### Определения функций

```cpp
ReturnType ClassName::functionName(ParamType1 param1, ParamType2 param2)
{
    if (condition)
    {
        doSomething();
    }
    else
    {
        doSomethingElse();
    }

    return result;
}
```

### Управляющие структуры

```cpp
// Операторы if
if (condition)
{
    action();
}

// Операторы switch
switch (value)
{
    case Value::First:
        handleFirst();
        break;

    case Value::Second:
        handleSecond();
        break;

    default:
        handleDefault();
        break;
}

// Циклы
for (size_t i = 0; i < count; ++i)
{
    process(items[i]);
}

for (const auto& item : container)
{
    process(item);
}

while (condition)
{
    iterate();
}
```

## Заголовочные файлы

### Защита заголовков

Используйте `#pragma once` для защиты заголовков:

```cpp
#pragma once

#include <cstdint>
#include "NovelMind/core/types.hpp"

namespace NovelMind::VFS
{
    // ...
}
```

### Порядок включений

1. Соответствующий заголовок (для .cpp файлов)
2. Заголовки стандартной библиотеки C++
3. Заголовки сторонних библиотек
4. Заголовки проекта (отсортированы по алфавиту)

```cpp
#include "scene_manager.hpp"

#include <algorithm>
#include <memory>
#include <vector>

#include <SDL.h>

#include "NovelMind/core/logger.hpp"
#include "NovelMind/renderer/sprite.hpp"
#include "NovelMind/vfs/resource.hpp"
```

### Прямые объявления

Предпочитайте прямые объявления в заголовках, где это возможно:

```cpp
// В заголовке
namespace NovelMind::Renderer
{
    class Texture;
    class Sprite;
}

class SceneManager
{
    std::unique_ptr<NovelMind::Renderer::Sprite> m_background;
};
```

## Классы и структуры

### Структура класса

```cpp
class ClassName
{
public:
    // Типы и псевдонимы
    using Ptr = std::unique_ptr<ClassName>;

    // Статические константы
    static constexpr int MAX_COUNT = 100;

    // Конструкторы и деструктор
    ClassName();
    explicit ClassName(int value);
    ~ClassName();

    // Операции копирования/перемещения
    ClassName(const ClassName&) = delete;
    ClassName& operator=(const ClassName&) = delete;
    ClassName(ClassName&&) noexcept;
    ClassName& operator=(ClassName&&) noexcept;

    // Публичный интерфейс
    void publicMethod();
    [[nodiscard]] int getValue() const;

protected:
    // Защищенный интерфейс
    void protectedMethod();

private:
    // Приватные методы
    void privateHelper();

    // Переменные-члены (всегда последними)
    int m_value;
    std::string m_name;
};
```

### Struct против Class

- Используйте `struct` для простых агрегатов данных (POD-подобные типы)
- Используйте `class` для типов с инвариантами и поведением

```cpp
// Агрегат данных
struct Vec2
{
    float x;
    float y;
};

// Тип с поведением
class Transform
{
public:
    void setPosition(const Vec2& pos);
    void setRotation(float angle);

private:
    Vec2 m_position;
    float m_rotation;
    bool m_dirty;
};
```

## Возможности современного C++

### Используйте умные указатели

```cpp
// Уникальное владение
std::unique_ptr<Texture> texture = std::make_unique<Texture>(path);

// Совместное владение (использовать осторожно)
std::shared_ptr<Resource> resource = std::make_shared<Resource>(id);

// Не владеющая ссылка
Texture* rawPtr = texture.get();
```

### Используйте `auto` разумно

```cpp
// Хорошо: очевидно из контекста или длинные имена типов
auto it = container.begin();
auto texture = resourceManager.loadTexture("sprite.png");

// Избегайте: скрывает важную информацию о типе
auto value = calculateValue();  // Какой это тип?
int value = calculateValue();   // Лучше
```

### Используйте `constexpr` и `const`

```cpp
constexpr int BUFFER_SIZE = 1024;
constexpr float PI = 3.14159265f;

const std::string& getName() const { return m_name; }
void process(const std::vector<int>& data);
```

### Используйте `[[nodiscard]]`

```cpp
[[nodiscard]] bool initialize();
[[nodiscard]] std::optional<Resource> loadResource(const std::string& id);
```

### Используйте классы перечислений

```cpp
enum class TextAlignment
{
    Left,
    Center,
    Right
};

// Использование
TextAlignment align = TextAlignment::Center;
```

## Обработка ошибок

### Без исключений

Исключения отключены в проекте. Используйте явную обработку ошибок:

```cpp
// Тип Result для операций, которые могут завершиться с ошибкой
template<typename T, typename E = std::string>
class Result
{
public:
    static Result ok(T value);
    static Result error(E error);

    [[nodiscard]] bool isOk() const;
    [[nodiscard]] bool isError() const;
    [[nodiscard]] T& value();
    [[nodiscard]] E& error();
};

// Использование
Result<Texture> result = loadTexture("image.png");
if (result.isOk())
{
    useTexture(result.value());
}
else
{
    log::error("Не удалось загрузить текстуру: {}", result.error());
}
```

### Утверждения

Используйте утверждения для ошибок программиста:

```cpp
#define NOVELMIND_ASSERT(condition, message) \
    do { if (!(condition)) { NovelMind::Core::assertFailed(#condition, message, __FILE__, __LINE__); } } while(0)

void processItem(Item* item)
{
    NOVELMIND_ASSERT(item != nullptr, "Item не должен быть null");
    // ...
}
```

## Документация

### Заголовки файлов

Каждый исходный файл должен иметь заголовочный комментарий:

```cpp
/**
 * @file scene_manager.hpp
 * @brief Управление сценами и их жизненным циклом
 *
 * Данный модуль предоставляет класс SceneManager, который обрабатывает
 * загрузку сцен, переходы и управление объектами.
 */
```

### Документация классов

```cpp
/**
 * @brief Управляет жизненным циклом сцен и переходами
 *
 * SceneManager отвечает за загрузку сцен, управление
 * объектами сцен и обработку переходов между сценами.
 */
class SceneManager
{
public:
    /**
     * @brief Загрузить сцену по идентификатору
     * @param sceneId Уникальный идентификатор сцены
     * @return Result, указывающий на успех или ошибку
     */
    [[nodiscard]] Result<void> loadScene(const std::string& sceneId);
};
```

### Встроенные комментарии

- Пишите комментарии для неочевидной логики
- Избегайте комментирования очевидного кода
- Поддерживайте комментарии актуальными при изменении кода

```cpp
// Хорошо: объясняет почему
// Используем размер степени двойки для атласа текстур для обеспечения совместимости с GPU
constexpr int ATLAS_SIZE = 2048;

// Плохо: констатирует очевидное
// Увеличиваем счетчик
++counter;
```

## Тестирование

### Именование тестовых файлов

- Юнит-тесты: `test_<имя_модуля>.cpp`
- Интеграционные тесты: `integration_<функция>.cpp`

### Структура теста

```cpp
#include <catch2/catch_test_macros.hpp>
#include "NovelMind/vfs/virtual_fs.hpp"

TEST_CASE("VirtualFS загружает ресурсы корректно", "[vfs]")
{
    NovelMind::VFS::VirtualFS vfs;

    SECTION("может загрузить существующий ресурс")
    {
        auto result = vfs.readFile("test_resource");
        REQUIRE(result.isOk());
    }

    SECTION("возвращает ошибку для отсутствующего ресурса")
    {
        auto result = vfs.readFile("nonexistent");
        REQUIRE(result.isError());
    }
}
```

## Конфигурация сборки

### Стандарты CMake

- Минимальная версия CMake: 3.20
- Используйте современные цели CMake
- Избегайте глобальных настроек

```cmake
target_compile_features(engine_core PUBLIC cxx_std_20)
target_compile_options(engine_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
```

### Уровни предупреждений

- Включайте максимальные предупреждения при разработке
- Рассматривайте предупреждения как ошибки в CI
- Документируйте любые подавленные предупреждения

## Чек-лист для ревью кода

Перед отправкой кода на ревью:

- [ ] Код следует соглашениям об именовании
- [ ] Нет магических чисел (используйте именованные константы)
- [ ] Публичный API документирован
- [ ] Обработаны случаи ошибок
- [ ] Нет утечек памяти (используется RAII)
- [ ] Написаны тесты для новой функциональности
- [ ] Нет предупреждений компилятора
- [ ] Код компилируется на всех целевых платформах
