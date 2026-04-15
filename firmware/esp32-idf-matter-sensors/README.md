# ESP-IDF Matter firmware for ESP32-WROOM DevKitV1

Проект реализует прошивку на чистом `ESP-IDF` без Arduino-слоя.

Возможности:

- `DS18B20` на `GPIO4`
- `AM2320` на `I2C SDA=19 / SCL=21`
- две Wi-Fi сети с автоматическим fallback: `Naiit-TP-Link` и `Tenda`
- локальный web-интерфейс диагностики и калибровки датчиков
- журнал состояний Wi-Fi / Matter / сенсоров
- подготовленная архитектура для интеграции `ESP-Matter`

## Важное ограничение

В текущем репозитории отсутствуют локально сохраненные исходники `ESP-Matter` / `ConnectedHomeIP`.
Поэтому проект собран так, чтобы:

1. полностью работать как ESP-IDF прошивка с сенсорами, web UI и логированием;
2. автоматически включать Matter-часть только если локальный компонент `esp_matter` будет добавлен в `components/esp_matter/`.

Без этого локального компонента прошивка собирается в режиме `Matter stub`, где:

- сохраняется API и журналирование событий Matter;
- web UI показывает состояние готовности Matter;
- реальное комиссионирование и публикация атрибутов не активируются.

## Структура

- `main/` - основной код приложения
- `components/am2320/` - локальный драйвер AM2320
- `components/onewire_bus/` - шина 1-Wire
- `components/ds18b20/` - локальный драйвер DS18B20

## Web UI

После подключения к Wi-Fi открыть:

- `http://<ip>/`
- `http://esp32-matter-sensors.local/` если mDNS доступен в сети

Доступные API:

- `GET /api/status`
- `GET /api/logs`
- `POST /api/calibration`
- `POST /api/sensors/read`

## Калибровка

Смещения сохраняются в `NVS` отдельно для:

- температуры `DS18B20`
- температуры `AM2320`
- влажности `AM2320`

## Подключение датчиков

- `GPIO4` -> `DS18B20 DATA` + подтяжка `4.7k` к `3.3V`
- `GPIO19` -> `AM2320 SDA`
- `GPIO21` -> `AM2320 SCL`

## Сборка

```bash
idf.py set-target esp32
idf.py build
```

## Включение реального Matter

### Опция 1: Использование ESP-IDF Component Registry (рекомендуется для стабильной версии)

```bash
cd firmware/esp32-idf-matter-sensors
idf.py add-dependency "espressif/esp_matter^1.4.2~1"
```

Это загрузит esp_matter v1.4.2~1 в директорию `components/`.

### Опция 2: Полный репозиторий ESP-Matter (для v1.5)

ДляMatter v1.5 используйте ветку `release/v1.5`:

```bash
git clone --branch release/v1.5 --depth 1 \
    https://github.com/espressif/esp-matter.git \
    components/esp_matter_temp
cd components/esp_matter_temp
git submodule update --init --depth 1
# Переместите esp_matter в нужное место
mv components/esp_matter components/esp_matter_full
# Удалите временную директорию
rm -rf components/esp_matter_temp
```

### После добавления компонента

Очистите и пересоберите проект:

```bash
idf.py fullclean
idf.py set-target esp32
idf.py build
```

После этого проект начнет использовать реальную реализацию вместо stub-слоя.
