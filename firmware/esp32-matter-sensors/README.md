# ESP32 Matter firmware

Прошивка для `ESP32-WROOM DevKitV1` с тремя Matter endpoint:

- `DS18B20` как отдельный датчик температуры на `GPIO4`
- `AM2320` как датчик температуры по `I2C SDA=19 / SCL=21`
- `AM2320` как датчик влажности по тому же I2C

Это позволяет добавить устройство в Matter-совместимый контроллер умного дома. В Яндекс умном доме поддержка зависит от текущего уровня поддержки Matter-контроллером и конкретных типов сенсоров на стороне Яндекса.

## Wi-Fi

В прошивке зашиты две сети:

- `Naiit-TP-Link`
- `Tenda`

Пароль для обеих: `24031993`

Подключение происходит по очереди к каждой сети до успешного соединения.

## Пины

- `DS18B20 DATA` -> `GPIO4`
- `AM2320 SDA` -> `GPIO19`
- `AM2320 SCL` -> `GPIO21`

Для `DS18B20` нужен подтягивающий резистор `4.7 кОм` между `DATA` и `3.3V`.

## Локальные библиотеки

Все нужные библиотеки сохранены в проекте локально в папке `third_party/`:

- `third_party/Adafruit_AM2320`
- `third_party/Adafruit_BusIO`
- `third_party/Adafruit_Sensor`
- `third_party/OneWire`
- `third_party/DallasTemperature`

## Сборка в Arduino IDE 2.x

1. Установить плату `ESP32 by Espressif Systems` версии `3.1.x` или новее с поддержкой Arduino Matter.
2. Открыть скетч `firmware/esp32-matter-sensors/esp32_matter_sensors.ino`.
3. Через `Sketch -> Add Folder to Sketch` или ручным копированием добавить локальные библиотеки из `third_party/` в каталог библиотек Arduino.
4. В меню платы выбрать:
   - Board: `ESP32 Dev Module`
   - Partition Scheme: `Huge APP (3MB No OTA/1MB SPIFFS)`
   - Erase All Flash Before Sketch Upload: `Enabled`
5. Прошить плату.

## Первый запуск

- После старта устройство выводит в Serial Monitor `115200` QR URL и manual pairing code.
- Добавление в Matter выполняется через контроллер умного дома.
- Кнопку `BOOT` можно удерживать более `5 секунд`, чтобы сбросить Matter-комиссионирование.

## Структура

- `esp32_matter_sensors.ino` - основная прошивка
- `README.md` - инструкция по подключению и прошивке
