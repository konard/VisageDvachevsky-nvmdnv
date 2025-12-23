# Спецификация ядра runtime

Этот документ определяет API и интерфейсы ядра runtime для движка NovelMind.

## Слой абстракции платформы

### Интерфейс окна

```cpp
namespace NovelMind::platform
{

struct WindowConfig
{
    std::string title = "NovelMind";
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    bool resizable = true;
    bool vsync = true;
};

class IWindow
{
public:
    virtual ~IWindow() = default;

    virtual bool create(const WindowConfig& config) = 0;
    virtual void destroy() = 0;

    virtual void setTitle(const std::string& title) = 0;
    virtual void setSize(int width, int height) = 0;
    virtual void setFullscreen(bool fullscreen) = 0;

    [[nodiscard]] virtual int getWidth() const = 0;
    [[nodiscard]] virtual int getHeight() const = 0;
    [[nodiscard]] virtual bool isFullscreen() const = 0;
    [[nodiscard]] virtual bool shouldClose() const = 0;

    virtual void pollEvents() = 0;
    virtual void swapBuffers() = 0;

    [[nodiscard]] virtual void* getNativeHandle() const = 0;
};

} // namespace NovelMind::platform
```

### Интерфейс таймера

```cpp
namespace NovelMind::platform
{

class ITimer
{
public:
    virtual ~ITimer() = default;

    virtual void reset() = 0;

    [[nodiscard]] virtual double getElapsedSeconds() const = 0;
    [[nodiscard]] virtual double getElapsedMilliseconds() const = 0;
    [[nodiscard]] virtual uint64_t getElapsedMicroseconds() const = 0;

    [[nodiscard]] virtual double getDeltaTime() const = 0;
    virtual void tick() = 0;
};

} // namespace NovelMind::platform
```

### Интерфейс файловой системы

```cpp
namespace NovelMind::platform
{

class IFileSystem
{
public:
    virtual ~IFileSystem() = default;

    [[nodiscard]] virtual Result<std::vector<uint8_t>> readFile(
        const std::string& path) const = 0;

    [[nodiscard]] virtual Result<void> writeFile(
        const std::string& path,
        const std::vector<uint8_t>& data) const = 0;

    [[nodiscard]] virtual bool exists(const std::string& path) const = 0;
    [[nodiscard]] virtual bool isFile(const std::string& path) const = 0;
    [[nodiscard]] virtual bool isDirectory(const std::string& path) const = 0;

    [[nodiscard]] virtual std::string getExecutablePath() const = 0;
    [[nodiscard]] virtual std::string getUserDataPath() const = 0;
};

} // namespace NovelMind::platform
```

## Виртуальная файловая система

### Интерфейс VFS

```cpp
namespace NovelMind::vfs
{

enum class ResourceType
{
    Unknown,
    Texture,
    Audio,
    Font,
    Script,
    Scene,
    Localization,
    Data
};

struct ResourceInfo
{
    std::string id;
    ResourceType type;
    size_t size;
    uint32_t checksum;
};

class IVirtualFileSystem
{
public:
    virtual ~IVirtualFileSystem() = default;

    virtual Result<void> mount(const std::string& packPath) = 0;
    virtual void unmount(const std::string& packPath) = 0;
    virtual void unmountAll() = 0;

    [[nodiscard]] virtual Result<std::vector<uint8_t>> readFile(
        const std::string& resourceId) const = 0;

    [[nodiscard]] virtual bool exists(const std::string& resourceId) const = 0;
    [[nodiscard]] virtual std::optional<ResourceInfo> getInfo(
        const std::string& resourceId) const = 0;

    [[nodiscard]] virtual std::vector<std::string> listResources(
        ResourceType type = ResourceType::Unknown) const = 0;
};

} // namespace NovelMind::vfs
```

### Дескриптор ресурса

```cpp
namespace NovelMind::vfs
{

template<typename T>
class ResourceHandle
{
public:
    ResourceHandle() = default;
    explicit ResourceHandle(std::shared_ptr<T> resource);

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] T* get() const;
    [[nodiscard]] T* operator->() const;
    [[nodiscard]] T& operator*() const;

    void reset();

private:
    std::shared_ptr<T> m_resource;
};

} // namespace NovelMind::vfs
```

## Менеджер ресурсов

### Интерфейс менеджера ресурсов

```cpp
namespace NovelMind::resource
{

class ResourceManager
{
public:
    explicit ResourceManager(NovelMind::vfs::IVirtualFileSystem* vfs = nullptr);
    ~ResourceManager();

    void setVfs(NovelMind::vfs::IVirtualFileSystem* vfs);
    void setBasePath(const std::string& path);

    // Загрузка текстур
    [[nodiscard]] Result<TextureHandle> loadTexture(const std::string& id);
    void unloadTexture(const std::string& id);

    // Загрузка шрифтов
    [[nodiscard]] Result<FontHandle> loadFont(const std::string& id, int size);
    void unloadFont(const std::string& id, int size);

    [[nodiscard]] Result<FontAtlasHandle>
    loadFontAtlas(const std::string& id, int size, const std::string& charset);

    // Универсальное чтение данных (аудио, скрипты, json)
    [[nodiscard]] Result<std::vector<uint8_t>>
    readData(const std::string& id) const;

    // Управление кешем
    void clearCache();

    [[nodiscard]] size_t getTextureCount() const;
    [[nodiscard]] size_t getFontCount() const;
    [[nodiscard]] size_t getFontAtlasCount() const;

private:
    NovelMind::vfs::IVirtualFileSystem* m_vfs;
    std::string m_basePath;
    // Внутренние кеши...
};

} // namespace NovelMind::resource
```

## Рендерер

### Интерфейс рендерера

```cpp
namespace NovelMind::renderer
{

struct Color
{
    uint8_t r, g, b, a;

    static const Color White;
    static const Color Black;
    static const Color Transparent;
};

struct Rect
{
    float x, y, width, height;
};

struct Transform2D
{
    float x = 0.0f;
    float y = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float rotation = 0.0f;
    float anchorX = 0.0f;
    float anchorY = 0.0f;
};

enum class BlendMode
{
    None,
    Alpha,
    Additive,
    Multiply
};

class IRenderer
{
public:
    virtual ~IRenderer() = default;

    virtual Result<void> initialize(NovelMind::platform::IWindow& window) = 0;
    virtual void shutdown() = 0;

    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    virtual void clear(const Color& color) = 0;

    virtual void setBlendMode(BlendMode mode) = 0;

    // Рендеринг спрайтов
    virtual void drawSprite(
        const Texture& texture,
        const Transform2D& transform,
        const Color& tint = Color::White) = 0;

    virtual void drawSprite(
        const Texture& texture,
        const Rect& sourceRect,
        const Transform2D& transform,
        const Color& tint = Color::White) = 0;

    // Рендеринг текста
    virtual void drawText(
        const Font& font,
        const std::string& text,
        float x, float y,
        const Color& color = Color::White) = 0;

    // Рендеринг примитивов
    virtual void drawRect(const Rect& rect, const Color& color) = 0;
    virtual void fillRect(const Rect& rect, const Color& color) = 0;

    // Эффекты экрана
    virtual void setFade(float alpha, const Color& color = Color::Black) = 0;

    [[nodiscard]] virtual int getWidth() const = 0;
    [[nodiscard]] virtual int getHeight() const = 0;
};

} // namespace NovelMind::renderer
```

### Текстура

```cpp
namespace NovelMind::renderer
{

class Texture
{
public:
    Texture() = default;
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    Result<void> loadFromMemory(const std::vector<uint8_t>& data);
    void destroy();

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] int getWidth() const;
    [[nodiscard]] int getHeight() const;
    [[nodiscard]] void* getNativeHandle() const;

private:
    void* m_handle = nullptr;
    int m_width = 0;
    int m_height = 0;
};

} // namespace NovelMind::renderer
```

## Движок скриптов

### Типы скриптов

```cpp
namespace NovelMind::scripting
{

enum class OpCode : uint8_t
{
    // Управление потоком
    NOP,
    HALT,
    JUMP,
    JUMP_IF,
    JUMP_IF_NOT,
    CALL,
    RETURN,

    // Операции со стеком
    PUSH_INT,
    PUSH_FLOAT,
    PUSH_STRING,
    PUSH_BOOL,
    PUSH_NULL,
    POP,
    DUP,

    // Переменные
    LOAD_VAR,
    STORE_VAR,
    LOAD_GLOBAL,
    STORE_GLOBAL,

    // Арифметика
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    NEG,

    // Сравнение
    EQ,
    NE,
    LT,
    LE,
    GT,
    GE,

    // Логика
    AND,
    OR,
    NOT,

    // Команды визуальной новеллы
    SHOW_BACKGROUND,
    SHOW_CHARACTER,
    HIDE_CHARACTER,
    SAY,
    CHOICE,
    SET_FLAG,
    CHECK_FLAG,
    PLAY_SOUND,
    PLAY_MUSIC,
    STOP_MUSIC,
    WAIT,
    TRANSITION,
    GOTO_SCENE
};

struct Instruction
{
    OpCode opcode;
    uint32_t operand;
};

} // namespace NovelMind::scripting
```

### Интерпретатор скриптов

```cpp
namespace NovelMind::scripting
{

class ScriptInterpreter
{
public:
    ScriptInterpreter();
    ~ScriptInterpreter();

    Result<void> loadScript(const std::vector<uint8_t>& bytecode);
    void reset();

    void setCallback(OpCode op, std::function<void(uint32_t)> callback);

    bool step();
    void run();
    void pause();
    void resume();

    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] bool isPaused() const;
    [[nodiscard]] bool isWaiting() const;

    void setVariable(const std::string& name, int value);
    void setVariable(const std::string& name, float value);
    void setVariable(const std::string& name, const std::string& value);
    void setVariable(const std::string& name, bool value);

    [[nodiscard]] std::optional<int> getIntVariable(const std::string& name) const;
    [[nodiscard]] std::optional<float> getFloatVariable(const std::string& name) const;
    [[nodiscard]] std::optional<std::string> getStringVariable(const std::string& name) const;
    [[nodiscard]] std::optional<bool> getBoolVariable(const std::string& name) const;

private:
    // Детали реализации
};

} // namespace NovelMind::scripting
```

## Система сцен

### Scene Graph

```cpp
namespace NovelMind::scene
{

enum class LayerType
{
    Background,
    Characters,
    UI,
    Effects
};

class SceneGraph
{
public:
    SceneGraph();
    ~SceneGraph();

    void setSceneId(const std::string& sceneId);
    void clear();

    void update(float deltaTime);
    void render(NovelMind::renderer::IRenderer& renderer);

    // Управление слоями
    void addToLayer(LayerType layer, std::unique_ptr<SceneObject> object);
    std::unique_ptr<SceneObject> removeFromLayer(LayerType layer,
                                                 const std::string& objectId);
    void clearLayer(LayerType layer);

    // Доступ к объектам
    SceneObject* findObject(const std::string& id);

private:
    // Детали реализации
};

} // namespace NovelMind::scene
```

### Объект сцены

```cpp
namespace NovelMind::scene
{

class SceneObject
{
public:
    explicit SceneObject(const std::string& id);
    virtual ~SceneObject() = default;

    [[nodiscard]] const std::string& getId() const;

    void setPosition(float x, float y);
    void setScale(float scaleX, float scaleY);
    void setRotation(float angle);
    void setAlpha(float alpha);
    void setVisible(bool visible);

    [[nodiscard]] const Transform2D& getTransform() const;
    [[nodiscard]] float getAlpha() const;
    [[nodiscard]] bool isVisible() const;

    virtual void update(float deltaTime);
    virtual void render(NovelMind::renderer::IRenderer& renderer) = 0;

protected:
    std::string m_id;
    Transform2D m_transform;
    float m_alpha = 1.0f;
    bool m_visible = true;
};

} // namespace NovelMind::scene
```

## Система ввода

### Менеджер ввода

```cpp
namespace NovelMind::input
{

enum class Key
{
    Unknown,
    Space,
    Enter,
    Escape,
    Up, Down, Left, Right,
    A, B, C, /* ... */ Z,
    Num0, Num1, /* ... */ Num9,
    F1, F2, /* ... */ F12,
    Shift, Ctrl, Alt
};

enum class MouseButton
{
    Left,
    Right,
    Middle
};

class InputManager
{
public:
    InputManager();
    ~InputManager();

    void update();

    // Клавиатура
    [[nodiscard]] bool isKeyDown(Key key) const;
    [[nodiscard]] bool isKeyPressed(Key key) const;
    [[nodiscard]] bool isKeyReleased(Key key) const;

    // Мышь
    [[nodiscard]] bool isMouseButtonDown(MouseButton button) const;
    [[nodiscard]] bool isMouseButtonPressed(MouseButton button) const;
    [[nodiscard]] bool isMouseButtonReleased(MouseButton button) const;
    [[nodiscard]] int getMouseX() const;
    [[nodiscard]] int getMouseY() const;

    // Текстовый ввод
    void startTextInput();
    void stopTextInput();
    [[nodiscard]] const std::string& getTextInput() const;

private:
    // Детали реализации
};

} // namespace NovelMind::input
```

## Аудио система

### Менеджер аудио

```cpp
namespace NovelMind::audio
{

class AudioManager
{
public:
    AudioManager();
    ~AudioManager();

    Result<void> initialize();
    void shutdown();

    // Источник данных (обычно ResourceManager/VFS)
    void setDataProvider(
        std::function<Result<std::vector<uint8_t>>(const std::string& id)> provider);

    // Звуковые эффекты
    AudioHandle playSound(const std::string& id,
                          const PlaybackConfig& config = {});
    void stopSound(AudioHandle handle, float fadeDuration = 0.0f);
    void stopAllSounds(float fadeDuration = 0.0f);

    // Музыка
    AudioHandle playMusic(const std::string& id, const MusicConfig& config = {});
    AudioHandle crossfadeMusic(const std::string& id, float duration,
                               const MusicConfig& config = {});
    void stopMusic(float fadeDuration = 0.0f);
    void pauseMusic();
    void resumeMusic();
    [[nodiscard]] bool isMusicPlaying() const;

    // Озвучка
    AudioHandle playVoice(const std::string& id, const VoiceConfig& config = {});
    void stopVoice(float fadeDuration = 0.0f);
    [[nodiscard]] bool isVoicePlaying() const;

    // Громкость
    void setMasterVolume(float volume);
    [[nodiscard]] float getMasterVolume() const;
    void setChannelVolume(AudioChannel channel, float volume);
    [[nodiscard]] float getChannelVolume(AudioChannel channel) const;

private:
    // Детали реализации
};

} // namespace NovelMind::audio
```

## Система сохранений

### Менеджер сохранений

```cpp
namespace NovelMind::save
{

struct SaveData
{
    std::string sceneId;
    std::string nodeId;
    std::map<std::string, int> intVariables;
    std::map<std::string, float> floatVariables;
    std::map<std::string, bool> flags;
    std::map<std::string, std::string> stringVariables;
    std::vector<uint8_t> thumbnailData;
    int thumbnailWidth = 0;
    int thumbnailHeight = 0;
    uint64_t timestamp;
    uint32_t checksum;
};

struct SaveMetadata
{
    uint64_t timestamp = 0;
    bool hasThumbnail = false;
    int thumbnailWidth = 0;
    int thumbnailHeight = 0;
    size_t thumbnailSize = 0;
};

struct SaveConfig
{
    bool enableCompression = true;
    bool enableEncryption = false;
    std::vector<uint8_t> encryptionKey; // 32 bytes for AES-256-GCM
};

class SaveManager
{
public:
    SaveManager();
    ~SaveManager();

    Result<void> save(int slot, const SaveData& data);
    Result<SaveData> load(int slot);
    Result<void> deleteSave(int slot);

    [[nodiscard]] bool slotExists(int slot) const;
    [[nodiscard]] std::optional<uint64_t> getSlotTimestamp(int slot) const;
    [[nodiscard]] std::optional<SaveMetadata> getSlotMetadata(int slot) const;

    [[nodiscard]] int getMaxSlots() const;

    void setSavePath(const std::string& path);
    [[nodiscard]] const std::string& getSavePath() const;

    void setConfig(const SaveConfig& config);
    [[nodiscard]] const SaveConfig& getConfig() const;

    Result<void> saveAuto(const SaveData& data);
    Result<SaveData> loadAuto();
    [[nodiscard]] bool autoSaveExists() const;

private:
    static constexpr int MAX_SLOTS = 100;
};

} // namespace NovelMind::save
```

## Основное приложение

### Класс приложения

```cpp
namespace NovelMind::core
{

struct EngineConfig
{
    platform::WindowConfig window;
    std::string packFile;
    std::string startScene;
    bool debug = false;
};

class Application
{
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    Result<void> initialize(const EngineConfig& config);
    void shutdown();

    void run();
    void quit();

    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] double getDeltaTime() const;
    [[nodiscard]] double getElapsedTime() const;

    [[nodiscard]] platform::IWindow* getWindow();
    [[nodiscard]] const platform::IWindow* getWindow() const;
    [[nodiscard]] platform::IFileSystem* getFileSystem();
    [[nodiscard]] renderer::IRenderer* getRenderer();
    [[nodiscard]] resource::ResourceManager* getResources();
    [[nodiscard]] scene::SceneGraph* getSceneGraph();
    [[nodiscard]] input::InputManager* getInput();
    [[nodiscard]] audio::AudioManager* getAudio();
    [[nodiscard]] save::SaveManager* getSaveManager();
    [[nodiscard]] localization::LocalizationManager* getLocalization();

protected:
    virtual void onInitialize();
    virtual void onShutdown();
    virtual void onUpdate(double deltaTime);
    virtual void onRender();

private:
    void mainLoop();

    bool m_running = false;
    EngineConfig m_config;

    // Подсистемы
    std::unique_ptr<platform::IWindow> m_window;
    std::unique_ptr<platform::IFileSystem> m_fileSystem;
    std::unique_ptr<vfs::IVirtualFileSystem> m_vfs;
    std::unique_ptr<renderer::IRenderer> m_renderer;
    std::unique_ptr<resource::ResourceManager> m_resources;
    std::unique_ptr<scene::SceneGraph> m_sceneGraph;
    std::unique_ptr<input::InputManager> m_input;
    std::unique_ptr<audio::AudioManager> m_audio;
    std::unique_ptr<save::SaveManager> m_saveManager;
    std::unique_ptr<localization::LocalizationManager> m_localization;
    Timer m_timer;
};

} // namespace NovelMind::core
```

## Система логирования

### Интерфейс логгера

```cpp
namespace NovelMind::core
{

enum class LogLevel
{
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

class Logger
{
public:
    static Logger& instance();

    void setLevel(LogLevel level);
    void setOutputFile(const std::string& path);

    template<typename... Args>
    void trace(const std::string& format, Args&&... args);

    template<typename... Args>
    void debug(const std::string& format, Args&&... args);

    template<typename... Args>
    void info(const std::string& format, Args&&... args);

    template<typename... Args>
    void warning(const std::string& format, Args&&... args);

    template<typename... Args>
    void error(const std::string& format, Args&&... args);

    template<typename... Args>
    void fatal(const std::string& format, Args&&... args);

private:
    Logger() = default;
    void log(LogLevel level, const std::string& message);
};

// Удобные макросы
#define NM_LOG_TRACE(...) NovelMind::core::Logger::instance().trace(__VA_ARGS__)
#define NM_LOG_DEBUG(...) NovelMind::core::Logger::instance().debug(__VA_ARGS__)
#define NM_LOG_INFO(...)  NovelMind::core::Logger::instance().info(__VA_ARGS__)
#define NM_LOG_WARN(...)  NovelMind::core::Logger::instance().warning(__VA_ARGS__)
#define NM_LOG_ERROR(...) NovelMind::core::Logger::instance().error(__VA_ARGS__)
#define NM_LOG_FATAL(...) NovelMind::core::Logger::instance().fatal(__VA_ARGS__)

} // namespace NovelMind::core
```
