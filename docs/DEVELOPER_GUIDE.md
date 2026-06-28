# Home Hub — Разработчик Документация

> Версия: 1.0
> Язык: C++17
> IDE: VS Code + PlatformIO
> Framework: Arduino (ESP32)
> Платформа: ESP32 (esp32doit-devkit-v1)

---

## Содержание

1. [Обзор архитектуры](#1-обзор-архитектуры)
2. [Структура проекта](#2-структура-проекта)
3. [Жизненный цикл приложения](#3-жизненный-цикл-приложения)
4. [Ядро (core)](#4-ядро-core)
   - 4.1 [Интерфейсы](#41-интерфейсы)
   - 4.2 [Типы](#42-типы)
   - 4.3 [Утилиты](#43-утилиты)
   - 4.4 [ServiceManager](#44-servicemanager)
   - 4.5 [ServiceRunner](#45-servicerunner)
5. [Сервисы (services)](#5-сервисы-services)
   - 5.1 [SerialLogger](#51-seriallogger)
   - 5.2 [WiFiService](#52-wifiservice)
   - 5.3 [BleProvisioningService](#53-bleprovisioningservice)
   - 5.4 [OTAServer](#54-otaserver)
   - 5.5 [HeartbeatService](#55-heartbeatservice)
6. [Приложение (app)](#6-приложение-app)
7. [Конфигурация](#7-конфигурация)
8. [Сборка и запуск](#8-сборка-и-запуск)
   - 8.1 [Все сервисы](#81-все-сервисы)
   - 8.2 [Отдельные сервисы](#82-отдельные-сервисы)
   - 8.3 [Тесты](#83-тесты)
9. [Как добавить новый сервис](#9-как-добавить-новый-сервис)
10. [Тестирование](#10-тестирование)
11. [Принципы и правила](#11-принципы-и-правила)
12. [Часто задаваемые вопросы](#12-часто-задаваемые-вопросы)

---

## 1. Обзор архитектуры

Home Hub — модульное embedded-приложение для ESP32. Архитектура построена на трёх ключевых принципах:

1. **Единый жизненный цикл** — все модули (сервисы) реализуют интерфейс `IService` с методами `begin()`, `update()`, `shutdown()`.
2. **Инверсия зависимостей** — модули зависят от интерфейсов, а не от реализаций.
3. **Явные зависимости** — запрещены Singleton, все зависимости передаются через конструктор.

### Слои архитектуры

```
┌────────────────────────────────────────────┐
│                Application                  │  ← корневой объект
├────────────────────────────────────────────┤
│              ServiceManager                 │  ← управляет массивом сервисов
├────────────────────────────────────────────┤
│  IService  (интерфейс)                      │
│  ILogger   (интерфейс, extends IService)    │
│  IBleProvisioningHandler (интерфейс)        │
├──────────┬──────────┬──────────┬───────────┤
│ Serial   │  WiFi    │   BLE    │    OTA    │  ← реализации сервисов
│ Logger   │  Service │Provision.│  Server   │
│          │          │ Service  │           │
├──────────┴──────────┴──────────┴───────────┤
│            HeartbeatService                 │
└────────────────────────────────────────────┘
         ↓        ↓        ↓        ↓
    ┌─────────────────────────────────┐
    │      Arduino / ESP32 HAL       │
    └─────────────────────────────────┘
```

**Правило зависимостей**: нижние слои никогда не знают о верхних. `Hardware` → `Drivers` → `Services` → `Application`. Каждый слой использует только нижележащие.

### Текущие сервисы

| Сервис | Описание | Зависимости |
|--------|----------|-------------|
| `SerialLogger` | Логирование через UART (Serial) | — |
| `WiFiService` | Wi-Fi подключение, хранение SSID/пароля в NVS, fallback AP | `ILogger` |
| `BleProvisioningService` | BLE-сервер для первичной настройки | `IBleProvisioningHandler` |
| `OTAServer` | OTA-обновление по воздуху | — |
| `HeartbeatService` | Периодический тик (1 сек) | — |

---

## 2. Структура проекта

```
home-hub/
├── include/
│   ├── Config.h              ← конфигурация (WiFi SSID, пароль, AP, OTA)
│   └── README
│
├── src/
│   ├── main.cpp              ← точка входа, композиция сервисов
│   │
│   ├── app/
│   │   ├── Application.h
│   │   └── Application.cpp   ← корневой объект приложения
│   │
│   ├── core/
│   │   ├── Core.h            ← агрегирующий заголовок (подключает всё ядро)
│   │   ├── ServiceManager.h/cpp    ← менеджер сервисов
│   │   ├── ServiceRunner.h         ← обёртка для одного сервиса
│   │   ├── StandaloneServiceExample.h  ← пример изолированного запуска
│   │   │
│   │   ├── interfaces/
│   │   │   ├── IService.h           ← begin/update/shutdown
│   │   │   ├── ILogger.h            ← info/warn/error
│   │   │   └── IBleProvisioningHandler.h  ← BLE → WiFi связь
│   │   │
│   │   ├── types/
│   │   │   ├── Result.h             ← enum class Result
│   │   │   ├── Types.h              ← Byte, Word, DWord
│   │   │   └── Version.h            ← структура Version
│   │   │
│   │   └── utils/
│   │       ├── NonCopyable.h        ← запрет копирования
│   │       ├── NonMovable.h         ← запрет перемещения
│   │       └── NetworkVisibility.h  ← inline-функция проверки видимости сети
│   │
│   └── services/
│       ├── bluetooth/
│       │   ├── BleProvisioningService.h/cpp
│       ├── heartbeat/
│       │   └── HeartbeatService.h/cpp
│       ├── logger/
│       │   └── SerialLogger.h/cpp
│       ├── network/
│       │   └── WiFiService.h/cpp
│       └── ota/
│           └── OTAServer.h/cpp
│
├── test/
│   ├── test_network_visibility.cpp  ← юнит-тест
│   └── README
│
├── docs/
│   └── DEVELOPER_GUIDE.md        ← этот файл
│
└── platformio.ini                 ← конфигурация сборки
```

### Назначение каталогов

| Каталог | Назначение |
|---------|------------|
| `include/` | Публичные заголовки (Config.h) |
| `src/app/` | Корневой объект приложения |
| `src/core/` | Ядро: интерфейсы, типы, менеджеры |
| `src/core/interfaces/` | Абстрактные интерфейсы |
| `src/core/types/` | Базовые типы (Result, Version, Byte...) |
| `src/core/utils/` | Вспомогательные классы |
| `src/services/` | Реализации сервисов (по подкаталогам) |
| `test/` | Юнит-тесты |
| `docs/` | Документация |

---

## 3. Жизненный цикл приложения

### Диаграмма последовательности

```
main()
  │
  ▼
Application::Application(logger, services[], count)
  │  └─ инициализация полей, никакой сложной работы
  │
  ▼
Application::begin()
  │
  ▼
ServiceManager::beginAll()
  │  для каждого сервиса: service->begin()
  │  если service->begin() != Ok → остановка с ошибкой
  │
  ▼
loop() бесконечно:
  │
  ▼
Application::update()
  │
  ▼
ServiceManager::updateAll()
  │  для каждого сервиса: service->update()
  │  если service->update() != Ok → остановка с ошибкой
  │
  ▼  (повтор)

При перезагрузке или выключении:
  Application::shutdown()
  ServiceManager::shutdownAll()
    для каждого сервиса (в обратном порядке): service->shutdown()
```

### Правила жизненного цикла

1. **Конструктор** — только базовая инициализация полей. Никакой работы с железом.
2. **begin()** — инициализация: открытие портов, подключение к WiFi, запуск BLE.
3. **update()** — периодическая работа. Вызывается в каждом цикле `loop()`.
4. **shutdown()** — освобождение ресурсов, отключение.

---

## 4. Ядро (core)

### 4.1 Интерфейсы

#### `IService` — базовый интерфейс всех сервисов

**Файл:** `src/core/interfaces/IService.h`

```cpp
namespace hub::core {

class IService {
public:
    virtual ~IService() = default;

    virtual Result begin()    = 0;  // инициализация
    virtual Result update()   = 0;  // периодическое обновление
    virtual Result shutdown() = 0;  // остановка
};

}
```

Поля класса | Описание
------------|----------
`begin()` | Вызывается один раз при старте. Возвращает `Result::Ok` при успехе.
`update()` | Вызывается в каждом цикле `loop()`. Должен быть неблокирующим.
`shutdown()` | Освобождает ресурсы. Вызывается при завершении.

#### `ILogger` — интерфейс логирования

**Файл:** `src/core/interfaces/ILogger.h`

```cpp
namespace hub::core {

class ILogger : public IService {
public:
    virtual void info(std::string_view message)  = 0;
    virtual void warn(std::string_view message)  = 0;
    virtual void error(std::string_view message) = 0;
};

}
```

Поля класса | Описание
------------|----------
`info()` | Информационное сообщение (префикс `[INFO]`)
`warn()` | Предупреждение (префикс `[WARN]`)
`error()` | Ошибка (префикс `[ERROR]`)

`ILogger` наследует `IService`, поэтому логгер участвует в жизненном цикле (begin/update/shutdown).

#### `IBleProvisioningHandler` — интерфейс для связи BLE → WiFi

**Файл:** `src/core/interfaces/IBleProvisioningHandler.h`

```cpp
namespace hub::core {

class IBleProvisioningHandler {
public:
    virtual ~IBleProvisioningHandler() = default;

    virtual void onProvisioningCredentials(const char* ssid, const char* password) = 0;
    virtual void onProvisioningScanRequest() = 0;
    virtual const char* consumeBleScanResponse() = 0;
};

}
```

Поля класса | Описание
------------|----------
`onProvisioningCredentials()` | Вызывается, когда BLE-клиент прислал SSID и пароль
`onProvisioningScanRequest()` | Вызывается при запросе сканирования WiFi-сетей
`consumeBleScanResponse()` | Возвращает и сбрасывает результат сканирования

Этот интерфейс позволяет `BleProvisioningService` не знать о `WiFiService` напрямую.

### 4.2 Типы

#### `Result` — коды возврата

**Файл:** `src/core/types/Result.h`

```cpp
enum class Result {
    Ok = 0,              // успех
    Error,               // общая ошибка
    InvalidArgument,     // неверный аргумент
    InvalidState,        // неверное состояние
    AlreadyInitialized,  // уже инициализировано
    NotInitialized,      // не инициализировано
    Timeout,             // таймаут
    Unsupported,         // не поддерживается
    OutOfMemory          // не хватает памяти
};
```

#### `Types.h` — базовые типы

**Файл:** `src/core/types/Types.h`

```cpp
using Byte  = std::uint8_t;
using Word  = std::uint16_t;
using DWord = std::uint32_t;
```

#### `Version` — версия

**Файл:** `src/core/types/Version.h`

```cpp
struct Version {
    std::uint16_t major;
    std::uint16_t minor;
    std::uint16_t patch;
};
```

### 4.3 Утилиты

#### `NonCopyable` — запрет копирования

**Файл:** `src/core/utils/NonCopyable.h`

```cpp
class NonCopyable {
protected:
    NonCopyable() = default;
    ~NonCopyable() = default;

public:
    NonCopyable(const NonCopyable&)            = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};
```

Используется для классов, которые не должны быть скопированы (например, `Application`).

#### `NonMovable` — запрет перемещения

**Файл:** `src/core/utils/NonMovable.h`

```cpp
class NonMovable {
protected:
    NonMovable() = default;
    ~NonMovable() = default;

public:
    NonMovable(NonMovable&&)            = delete;
    NonMovable& operator=(NonMovable&&) = delete;
};
```

#### `NetworkVisibility` — проверка видимости сети

**Файл:** `src/core/utils/NetworkVisibility.h`

```cpp
inline bool isHomeNetworkVisible(
    const std::string& savedSsid,
    const std::vector<std::string>& visibleNetworks
);
```

| Параметр | Описание |
|----------|----------|
| `savedSsid` | SSID сохранённой домашней сети |
| `visibleNetworks` | Список видимых WiFi-сетей |
| **Возвращает** | `true`, если `savedSsid` есть в `visibleNetworks` |

Функция inline, не требует компиляции отдельного .cpp файла. Это pure-функция без side-эффектов — удобно для тестирования.

### 4.4 ServiceManager

**Файлы:** `src/core/ServiceManager.h`, `src/core/ServiceManager.cpp`

```cpp
namespace hub::core {

class ServiceManager {
public:
    ServiceManager(IService* const* services, std::size_t count) noexcept;

    Result beginAll();     // инициализировать все сервисы
    Result updateAll();    // обновить все сервисы
    Result shutdownAll();  // остановить все сервисы
};

}
```

**Принципы работы:**

1. Принимает массив указателей на `IService` и количество.
2. `beginAll()` итерирует сервисы по порядку. При ошибке прерывает цепочку.
3. `updateAll()` итерирует все сервисы. При ошибке прерывает цепочку.
4. `shutdownAll()` итерирует все сервисы.
5. **Не владеет сервисами** — не управляет их памятью.
6. Сервисы должны быть статически аллоцированы (глобальные объекты).

**Пример порядка в массиве определяет порядок инициализации:**
```cpp
static hub::core::IService* services[] = {
    &logger,        // 1. Логгер — должен быть первым
    &wifiService,   // 2. WiFi — зависит от логгера
    &bleService,    // 3. BLE — зависит от WiFi (через handler)
    &otaServer,     // 4. OTA
    &heartbeatService, // 5. Heartbeat
};
```

### 4.5 ServiceRunner

**Файл:** `src/core/ServiceRunner.h`

```cpp
namespace hub::core {

class ServiceRunner {
public:
    explicit ServiceRunner(IService& service) noexcept;

    Result runOnce();   // вызывает begin() при первом вызове, затем update()
    Result shutdown();  // вызывает shutdown() однократно
};

}
```

**Назначение:** Упрощает изолированный запуск одного сервиса (например, для тестирования). При первом вызове `runOnce()` вызывается `begin()`, при последующих — `update()`.

**Пример использования в StandaloneServiceExample:**
```cpp
class StandaloneServiceExample {
    ServiceRunner m_runner;
    // При первом вызове begin() → WiFiService::begin()
    // При последующих update() → WiFiService::update()
    Result begin()  { return m_runner.runOnce(); }
    Result update() { return m_runner.runOnce(); }
};
```

---

## 5. Сервисы (services)

### 5.1 SerialLogger

**Файлы:** `src/services/logger/SerialLogger.h`, `src/services/logger/SerialLogger.cpp`

**Назначение:** Логирование через UART (Serial) с цветовой маркировкой уровней.

```cpp
namespace hub::services {

class SerialLogger final : public hub::core::ILogger {
public:
    SerialLogger() = default;

    hub::core::Result begin()    override;  // Serial.begin(115200)
    hub::core::Result update()   override;  // ничего не делает (Ok)
    hub::core::Result shutdown() override;  // Serial.end()

    void info(std::string_view message)  override;  // [INFO]  сообщение
    void warn(std::string_view message)  override;  // [WARN]  сообщение
    void error(std::string_view message) override;  // [ERROR] сообщение
};

}
```

**Детали реализации:**

- `begin()` вызывает `Serial.begin(config::SerialBaudRate)`
- `shutdown()` вызывает `Serial.end()`
- `update()` ничего не делает, но обязан быть реализован (Ok)
- Каждое сообщение выводится с префиксом: `[INFO]`, `[WARN]`, `[ERROR]`
- Использует `std::string_view` для избежания копирования строк

**Baud rate:** 115200 (задаётся в `Config.h` через `config::SerialBaudRate`)

### 5.2 WiFiService

**Файлы:** `src/services/network/WiFiService.h`, `src/services/network/WiFiService.cpp`

**Назначение:** Управление Wi-Fi подключением. Реализует конечный автомат с поддержкой сохранения SSID/пароля в NVS, fallback в режим точки доступа, и взаимодействие с BLE-провижинингом.

```cpp
namespace hub::services {

class WiFiService final : public hub::core::IService
                        , public hub::core::IBleProvisioningHandler {
public:
    enum class ConnectionState {
        Booting,      // начальное состояние
        Connecting,   // попытка подключения
        Connected,    // успешно подключён
        Recovering,   // потеря связи, попытка восстановления
        SetupMode     // режим настройки (AP + BLE fallback)
    };

    explicit WiFiService(hub::core::ILogger& logger) noexcept;

    // IService
    hub::core::Result begin() override;
    hub::core::Result update() override;
    hub::core::Result shutdown() override;

    // IBleProvisioningHandler
    void onProvisioningCredentials(const char* ssid, const char* password) override;
    void onProvisioningScanRequest() override;
    const char* consumeBleScanResponse() override;

    // Публичные методы
    void resetStoredWiFiSettings();      // сбросить сохранённые настройки
    ConnectionState state() const;       // текущее состояние
    bool requiresSetup() const;          // true, если в режиме настройки
    const String& lastErrorMessage() const;
    const String& lastBleResponse() const;
};

}
```

#### Конечный автомат

```
     ┌──────────┐
     │  Booting  │  ← начальное состояние при запуске
     └─────┬─────┘
           │
     ┌─────▼──────┐
     │ Connecting  │  ← попытка подключения к WiFi
     └─────┬──────┘
           │
     ┌─────▼──────┐
     │ Connected   │  ← успешное подключение
     └─────┬──────┘
           │
     ┌─────▼───────┐
     │ Recovering  │  ← потеря связи, попытка восстановления
     └─────┬───────┘
           │
     ┌─────▼──────┐
     │ SetupMode  │  ← AP + BLE fallback (не удалось восстановить)
     └────────────┘
```

**Переходы:**

- `Booting → Connecting`: после загрузки сохранённого SSID/пароля
- `Booting → SetupMode`: если нет сохранённых настроек
- `Connecting → Connected`: WiFi.status() == WL_CONNECTED
- `Connected → Recovering`: потеря соединения
- `Recovering → Connecting`: повторная попытка подключения
- `Recovering → SetupMode`: исчерпаны попытки восстановления (-5), сеть не видна
- `SetupMode → Connecting`: получены новые SSID/пароль через BLE

#### Работа с NVS (Preferences)

WiFiService использует `Preferences` для хранения SSID и пароля в энергонезависимой памяти ESP32.

**Ключи в NVS (namespace "wifi"):**

| Ключ | Тип | Описание |
|------|-----|----------|
| `configured` | bool | Были ли сохранены настройки |
| `ssid` | String | SSID сети |
| `password` | String | Пароль сети |

**Методы работы с NVS:**

```cpp
void loadStoredConfiguration();   // чтение из NVS
void saveStoredConfiguration();   // запись в NVS
void clearStoredConfiguration();  // очистка NVS
```

#### Кнопка сброса (GPIO0)

- При загрузке, если GPIO0 замкнут на GND (кнопка нажата), WiFiService:
  1. Очищает сохранённые настройки
  2. Переходит в режим SetupMode
  3. Перезагружает устройство (ожидает BLE-настройки)

#### Debug-команды (удалены)

Ранее поддерживались команды через Serial-монитор. Удалены как избыточные. При необходимости можно вернуть через `#ifdef DEBUG`.

### 5.3 BleProvisioningService

**Файлы:** `src/services/bluetooth/BleProvisioningService.h`, `src/services/bluetooth/BleProvisioningService.cpp`

**Назначение:** BLE-сервер для первичной настройки Wi-Fi. Позволяет мобильному приложению подключиться к ESP32 по BLE и передать SSID и пароль WiFi-сети.

```cpp
namespace hub::services {

class BleProvisioningService final : public hub::core::IService {
public:
    explicit BleProvisioningService(hub::core::IBleProvisioningHandler& handler) noexcept;

    hub::core::Result begin() override;
    hub::core::Result update() override;
    hub::core::Result shutdown() override;

    void setResponse(const String& response);
    bool isActive() const;
};

}
```

#### BLE-характеристики

| Параметр | Значение |
|----------|----------|
| Device Name | `"HomeHub-Setup"` |
| Service UUID | `4fafc201-1fb5-459e-8fcc-c5c9c5c5d4b0` |
| Characteristic UUID | `beb5483e-36e1-4688-bd8c-ccde5f4e4f0d` |
| Свойства | READ, WRITE |

#### Протокол обмена

**Запрос сканирования:** клиент пишет `"SCAN"` → сервис отвечает `"SCAN_REQUESTED"`, затем асинхронно отправляет список сетей через `notify()` в формате `"SSID1;SSID2;SSID3"` или `"NO_NETWORKS"`.

**Передача учётных данных:** клиент пишет `"SSID|PASSWORD"` → сервис отвечает `"OK"` или `"ERR:format"` (если нет разделителя `|`).

#### Архитектура связи

```
  BLE-клиент                 BleProvisioningService           WiFiService
    │                               │                              │
    │──── "SCAN" ──────────────────►│                              │
    │                               │──── onProvisioningScanRequest() ──►│
    │◄─── "SCAN_REQUESTED" ────────│                              │
    │                               │                              │── WiFi.scanNetworks()
    │                               │◄─── consumeBleScanResponse() ──│
    │◄─── notify("SSID1;SSID2") ───│                              │
    │                               │                              │
    │──── "MyWiFi|password123" ───►│                              │
    │                               │──── onProvisioningCredentials() ──►│
    │◄─── "OK" ────────────────────│                              │── save + connect
```

#### Потокобезопасность

Колбэки BLE (onWrite) могут вызываться из контекста BLE-стека. Все обращения к `m_handler` защищены тем, что вызовы `onProvisioningCredentials` и `onProvisioningScanRequest` неблокирующие.

### 5.4 OTAServer

**Файлы:** `src/services/ota/OTAServer.h`, `src/services/ota/OTAServer.cpp`

**Назначение:** OTA-обновление прошивки по воздуху через встроенную библиотеку ArduinoOTA.

```cpp
namespace hub::services {

class OTAServer final : public hub::core::IService {
public:
    OTAServer() = default;

    hub::core::Result begin() override;
    hub::core::Result update() override;   // вызывает ArduinoOTA.handle()
    hub::core::Result shutdown() override;
};

}
```

#### Настройки OTA

| Параметр | Значение | Откуда берётся |
|----------|----------|----------------|
| Hostname | `"home-hub"` | `config::OTAHostname` (из `Config.h`) |
| Password | `"homehub"` | `config::OTAPassword` (из `Config.h`) |

#### Колбэки

```cpp
ArduinoOTA.onStart([]() {
    // Вызывается перед началом OTA-обновления
    // Здесь можно остановить критичные сервисы
});

ArduinoOTA.onEnd([]() {
    // Вызывается после завершения OTA
});

ArduinoOTA.onError([](ota_error_t error) {
    // Вызывается при ошибке OTA
});
```

Колбэки пока пустые. Рекомендуется добавить в `onStart` остановку WiFi и BLE-сервисов, а в `onEnd` — их перезапуск.

### 5.5 HeartbeatService

**Файлы:** `src/services/heartbeat/HeartbeatService.h`, `src/services/heartbeat/HeartbeatService.cpp`

**Назначение:** Периодический тик (каждую секунду) + сброс аппаратного Watchdog (TWDT). Если `feed()` не вызывается в течение таймаута (10 секунд), ESP32 перезагружается.

```cpp
namespace hub::services {

class HeartbeatService final : public hub::core::IService {
public:
    HeartbeatService() = default;

    hub::core::Result begin() override;
    hub::core::Result update() override;
    hub::core::Result shutdown() override;

private:
    unsigned long m_lastTick = 0;
    hub::core::utils::WatchdogManager m_watchdog;
};

}
```

**Логика:**
```cpp
Result HeartbeatService::begin() {
    m_lastTick = millis();
    m_watchdog.begin();  // инициализация TWDT с таймаутом 10 сек
    return Result::Ok;
}

Result HeartbeatService::update() {
    m_watchdog.feed();   // сброс watchdog — если сервис зависнет, ESP32 перезагрузится

    if (millis() - m_lastTick >= 1000) {
        m_lastTick = millis();
        // TODO: мигание LED, отправка keepalive и т.д.
    }
    return Result::Ok;
}

Result HeartbeatService::shutdown() {
    m_watchdog.end();    // остановка watchdog
    return Result::Ok;
}
```

#### WatchdogManager

**Файлы:** `src/core/utils/WatchdogManager.h`, `src/core/utils/WatchdogManager.cpp`

Обёртка над ESP32 Task Watchdog Timer (TWDT).

```cpp
namespace hub::core::utils {

class WatchdogManager {
public:
    explicit WatchdogManager(std::uint32_t timeoutSec = 10) noexcept;
    void begin();  // инициализация TWDT
    void feed();   // сброс таймера
    void end();    // остановка
};

}
```

| Метод | Описание |
|-------|----------|
| `begin()` | Инициализирует TWDT с таймаутом timeoutSec секунд. Добавляет текущую задачу (loop) под надзор. |
| `feed()` | Сбрасывает таймер. Должен вызываться чаще, чем timeoutSec. |
| `end()` | Останавливает watchdog. Вызывается в shutdown(). |

Если `feed()` не вызывается в течение таймаута, ESP32 аппаратно перезагружается. Это защищает от зависаний в loop().

---

## 6. Приложение (app)

**Файлы:** `src/app/Application.h`, `src/app/Application.cpp`

**Назначение:** Корневой объект приложения. Владеет логгером и менеджером сервисов.

```cpp
namespace hub::app {

class Application : public hub::core::NonCopyable {
public:
    Application(hub::core::ILogger& logger,
                hub::core::IService* const* services,
                std::size_t serviceCount) noexcept;

    hub::core::Result begin();
    hub::core::Result update();
    hub::core::Result shutdown();

private:
    hub::core::ILogger&       m_logger;
    hub::core::ServiceManager m_serviceManager;
};

}
```

### `main.cpp` — условная компиляция

**Файл:** `src/main.cpp`

Содержит условную компиляцию для выбора набора сервисов. Определяется флагом `-DSERVICE_<ИМЯ>`.

**Доступные флаги:**

| Флаг | Собираемые сервисы |
|------|-------------------|
| *(нет флага)* | Все сервисы |
| `SERVICE_BLE` | Logger + WiFiService + BleProvisioningService |
| `SERVICE_WIFI` | Logger + WiFiService |
| `SERVICE_OTA` | Logger + OTAServer |
| `SERVICE_HEARTBEAT` | Logger + HeartbeatService |

**Структура условной компиляции:**

```cpp
static hub::services::SerialLogger logger;

#if defined(SERVICE_BLE)
    // BLE + WiFi (нужен для сканирования)
    static hub::services::WiFiService wifiService(logger);
    static hub::services::BleProvisioningService bleService(wifiService);
    static hub::core::IService* services[] = { &logger, &bleService };

#elif defined(SERVICE_WIFI)
    static hub::services::WiFiService wifiService(logger);
    static hub::core::IService* services[] = { &logger, &wifiService };

#elif defined(SERVICE_OTA)
    static hub::services::OTAServer otaServer;
    static hub::core::IService* services[] = { &logger, &otaServer };

#elif defined(SERVICE_HEARTBEAT)
    static hub::services::HeartbeatService heartbeatService;
    static hub::core::IService* services[] = { &logger, &heartbeatService };

#else
    // Все сервисы
    static hub::services::WiFiService wifiService(logger);
    static hub::services::BleProvisioningService bleService(wifiService);
    static hub::services::OTAServer otaServer;
    static hub::services::HeartbeatService heartbeatService;
    static hub::core::IService* services[] = {
        &logger, &wifiService, &bleService, &otaServer, &heartbeatService,
    };
#endif

static hub::app::Application app(logger, services, count);
void setup() { app.begin(); }
void loop()  { app.update(); }
```

---

## 7. Конфигурация

**Файл:** `include/Config.h`

Все настройки проекта в одном месте.

```cpp
namespace config {
    constexpr std::uint32_t SerialBaudRate = 115200;

    // Wi-Fi (значения по умолчанию, переопределяются в platformio.ini)
    constexpr const char* WiFiSsid          = WIFI_SSID;       // "YOUR_SSID"
    constexpr const char* WiFiPassword      = WIFI_PASSWORD;   // "YOUR_PASSWORD"

    // OTA
    constexpr const char* OTAHostname       = OTA_HOSTNAME;    // "home-hub"
    constexpr const char* OTAPassword       = OTA_PASSWORD;    // "homehub"

    // Точка доступа (fallback)
    constexpr const char* AccessPointSsid   = AP_SSID;         // "HomeHub-Setup"
    constexpr const char* AccessPointPassword = AP_PASSWORD;   // "homehub123"
}
```

**Важно:** Значения `WIFI_SSID`, `WIFI_PASSWORD` и т.д. — это макросы, которые передаются через `build_flags` в `platformio.ini`. Это позволяет не хранить чувствительные данные в исходниках.

**Переопределение в platformio.ini:**
```ini
build_flags =
    -DWIFI_SSID=\"MyHomeWiFi\"
    -DWIFI_PASSWORD=\"MySecretPassword\"
```

---

## 8. Сборка и запуск

**Требования:**
- VS Code с расширением PlatformIO
- PlatformIO CLI (`~/.platformio/penv/bin/platformio`)
- ESP32 board (esp32doit-devkit-v1)

### 8.1 Все сервисы

```bash
# Debug-сборка (по умолчанию)
platformio run -e esp32dev-debug

# Release-сборка
platformio run -e esp32dev-release

# Загрузка на устройство
platformio run -e esp32dev-debug -t upload

# Монитор порта
platformio device monitor
```

### 8.2 Отдельные сервисы

```bash
# Только BLE
platformio run -e esp32dev-ble -t upload

# Только WiFi
platformio run -e esp32dev-wifi -t upload

# Только OTA
platformio run -e esp32dev-ota -t upload

# Только Heartbeat
platformio run -e esp32dev-heartbeat -t upload
```

### 8.3 Тесты

```bash
platformio test -e native-test
```

---

## 9. Как добавить новый сервис

### Шаг 1: Создать интерфейс (если нужно)

Если новый сервис будут использовать другие сервисы через абстракцию, создайте интерфейс в `src/core/interfaces/`:

```cpp
// src/core/interfaces/IMyNewService.h
#pragma once
#include "../types/Result.h"

namespace hub::core {

class IMyNewService {
public:
    virtual ~IMyNewService() = default;
    virtual Result doSomething(int param) = 0;
};

}
```

### Шаг 2: Реализовать сервис

Создайте файлы в `src/services/<category>/`:

```cpp
// src/services/mynew/MyNewService.h
#pragma once
#include "../../core/interfaces/IService.h"

namespace hub::services {

class MyNewService final : public hub::core::IService {
public:
    MyNewService() = default;

    hub::core::Result begin() override;
    hub::core::Result update() override;
    hub::core::Result shutdown() override;
};

}
```

```cpp
// src/services/mynew/MyNewService.cpp
#include "MyNewService.h"

namespace hub::services {

hub::core::Result MyNewService::begin() {
    // инициализация
    return hub::core::Result::Ok;
}

hub::core::Result MyNewService::update() {
    // периодическая работа
    return hub::core::Result::Ok;
}

hub::core::Result MyNewService::shutdown() {
    // освобождение ресурсов
    return hub::core::Result::Ok;
}

}
```

### Шаг 3: Добавить в main.cpp

```cpp
// В секцию "все сервисы"
#include "services/mynew/MyNewService.h"

#if defined(SERVICE_MYNEW)
static hub::services::MyNewService myNewService;
static hub::core::IService* services[] = { &logger, &myNewService };
#else
// ...
static hub::services::MyNewService myNewService;
static hub::core::IService* services[] = {
    &logger, &wifiService, &bleService, &otaServer, &heartbeatService, &myNewService,
};
#endif
```

### Шаг 4: Добавить environment в platformio.ini

```ini
[env:esp32dev-mynew]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200
build_flags = -std=gnu++17 -DSERVICE_MYNEW
board_build.partitions = huge_app.csv
```

### Шаг 5: Написать тесты

Создайте файл `test/test_mynew_service.cpp`:

```cpp
#include <unity.h>
#include "src/services/mynew/MyNewService.h"

void test_mynew_service_begin_returns_ok() {
    hub::services::MyNewService service;
    TEST_ASSERT_EQUAL(static_cast<int>(hub::core::Result::Ok),
                      static_cast<int>(service.begin()));
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_mynew_service_begin_returns_ok);
    UNITY_END();
}

void loop() {}
```

### Правила для новых сервисов

1. Наследуйте `IService` (или `ILogger`/`IBleProvisioningHandler` при необходимости)
2. Не используйте `Serial.println()` — только `ILogger`
3. Не создавайте сервисы внутри других сервисов — используйте `IBleProvisioningHandler`-подобные интерфейсы
4. Конструктор не должен делать сложную работу (ни delay, ни инициализация железа)
5. `update()` должен быть неблокирующим (без `delay()`)
6. Статическая аллокация — без `new`/`delete`
7. Явные зависимости через конструктор — без Singleton
8. Документируйте публичные методы в стиле Doxygen

---

## 10. Тестирование

### Фреймворк

Используется [Unity Test](https://github.com/ThrowTheSwitch/Unity) — легковесный C/C++ тестовый фреймворк.

### Запуск тестов

```bash
platformio test -e native-test
```

### Текущие тесты

| Файл | Описание | Количество тестов |
|------|----------|-------------------|
| `test_network_visibility.cpp` | Проверка функции `isHomeNetworkVisible()` | 3 |

### Написание тестов

```cpp
#include <unity.h>
#include <vector>
#include <string>
#include "src/core/utils/NetworkVisibility.h"

void test_example(void) {
    TEST_ASSERT_TRUE(hub::core::utils::isHomeNetworkVisible("MyWiFi", {"MyWiFi"}));
    TEST_ASSERT_FALSE(hub::core::utils::isHomeNetworkVisible("", {"MyWiFi"}));
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_example);
    return UNITY_END();
}
```

### Рекомендации

- Тестируйте pure-функции (без side-эффектов) — они легко тестируются
- Для тестирования сервисов используйте `NullLogger` из `StandaloneServiceExample.h`
- Для интеграционных тестов собирайте отдельный environment

---

## 11. Принципы и правила

### 11.1. Архитектурные принципы

1. **Application является корневым объектом системы.** Application владеет всеми сервисами и управляет их жизненным циклом. Никакой другой объект не создает сервисы.

2. **Один владелец — один объект.** Каждый сервис существует в единственном экземпляре. Запрещается создавать сервис внутри другого сервиса.

3. **Конструкторы не выполняют сложную работу.** Конструктор отвечает только за создание корректного объекта. Запрещается в конструкторе: подключаться к Wi-Fi, запускать OTA, открывать Serial, создавать задачи, выполнять длительные операции.

4. **Единый жизненный цикл.** Каждый сервис реализует `begin()`, `update()`, `shutdown()`.

5. **main.cpp содержит только запуск приложения.** Никакой бизнес-логики. Допустимый объём — до 40 строк.

6. **Запрещены Singleton.** Все зависимости должны быть явными, передаваться через конструктор.

7. **Запрещено динамическое выделение памяти без необходимости.** Не использовать `new`/`delete`, если задача решается статическим размещением объектов.

8. **Каждый модуль имеет одну ответственность.** Никаких "универсальных" классов.

9. **Архитектурные решения документируются.** Каждое важное решение фиксируется в документации.

10. **Архитектура важнее реализации.** Сначала проектируется интерфейс, потом реализация. Никогда наоборот.

### 11.2. Стиль кода

| Элемент | Стиль | Пример |
|---------|-------|--------|
| Классы | PascalCase | `WiFiService`, `SerialLogger` |
| Методы | camelCase | `connectToNetwork()`, `begin()` |
| Поля класса | m_camelCase | `m_logger`, `m_isConnected` |
| Константы | kPascalCase | `kRetryIntervalMs`, `kMaxFailedAttempts` |
| Перечисления | PascalCase | `ConnectionState::Connected` |
| Пространства имён | lowercase | `hub::services`, `hub::core` |
| Файлы | PascalCase.cpp/h | `WiFiService.cpp` |

### 11.3. Логирование

- Запрещено использовать `Serial.println()` напрямую
- Всегда используйте `ILogger` (внедрённый через конструктор)
- Уровни: `info()` для обычных событий, `warn()` для проблем, `error()` для ошибок

### 11.4. Конфигурация

- Все настройки находятся только в `Config.h`
- Никаких "магических чисел" внутри кода
- Чувствительные данные (SSID, пароли) передаются через build_flags

### 11.5. Память

- Предпочтительно статические объекты
- Избегать динамического выделения памяти во время работы программы
- Никаких `new`/`delete` без объективной причины

---

## 12. Часто задаваемые вопросы

### Как добавить новый сервис?

См. раздел [9. Как добавить новый сервис](#9-как-добавить-новый-сервис).

### Как запустить только BLE?

```bash
platformio run -e esp32dev-ble -t upload
```

### Как изменить SSID и пароль Wi-Fi?

Отредактируйте `platformio.ini`, секция `build_flags`:
```ini
build_flags =
    -DWIFI_SSID=\"MyWiFi\"
    -DWIFI_PASSWORD=\"MyPassword\"
```

### Где хранятся сохранённые настройки Wi-Fi?

В NVS (Non-Volatile Storage) ESP32, namespace `"wifi"`. Используется библиотека `Preferences`.

### Как сбросить настройки Wi-Fi?

Удерживайте кнопку на GPIO0 при включении (или нажмите после старта). WiFiService обнаружит LOW на пине и очистит NVS.

### Почему BLE не стартует?

BLE стартует автоматически только когда:
1. Нет сохранённых Wi-Fi настроек (первичная настройка)
2. Вы собрали с `-DSERVICE_BLE`

В обычном режиме BLE не запускается — только Wi-Fi и OTA.

### Как отладить если Wi-Fi не подключается?

1. Проверьте, что в `platformio.ini` указаны правильные SSID и пароль
2. Соберите с `-e esp32dev-debug` и смотрите логи через монитор порта
3. Проверьте, что ESP32 видит вашу Wi-Fi сеть (сигнал достаточной силы)

### Как работает fallback AP?

Если ESP32 не может подключиться к домашней сети после 5 попыток и 3 циклов восстановления, он:
1. Включает точку доступа `"HomeHub-Setup"` (пароль `"homehub123"`)
2. Переходит в режим настройки
3. Ожидает подключения по BLE для получения новых SSID/пароля

### Нужен ли мне BLE для работы?

Нет. Если у вас уже настроен Wi-Fi (SSID/пароль сохранены в NVS), BLE не нужен. ESP32 подключится к Wi-Fi при старте, и вы можете использовать OTA для обновлений.

### Как обновить прошивку по OTA?

1. Соберите новую прошивку: `platformio run -e esp32dev-debug`
2. Загрузите по OTA:
   ```bash
   platformio run -e esp32dev-debug -t upload --upload-port <IP_адрес>
   ```
   Или через PlatformIO: в панели Tasks → esp32dev-debug → Platform → Upload and Monitor

### Какие пины используются?

| Пин | Назначение |
|-----|------------|
| GPIO0 | Кнопка сброса Wi-Fi (INPUT_PULLUP, LOW = сброс) |
| UART0 (TX/RX) | Логирование (Serial) |

---

*Документ создан: 28.06.2026*
*Последнее обновление: 28.06.2026*