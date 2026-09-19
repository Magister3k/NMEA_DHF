# NMEA decoder for Borland C++Builder 6

Этот каталог содержит разбор входных предложений NMEA для DLL-модуля DHF.
Производственная сборка рассчитана на классический компилятор Borland
C++Builder 6 (BC++): код использует только возможности C++98 и не требует
`std::function`, лямбд, `auto`, `std::chrono` или других библиотек C++11.

## Состав DLL

В проект DLL включены только следующие исходные файлы:

- `../nmea_decoder.cpp` — реализация экспорта `getIModuleExt` и интеграция с DHF;
- `nmea0183_parser.cpp` — проверка XOR-контрольной суммы NMEA 0183;
- `nmea450_parser.cpp` — разбор IEC 61162-450 TAG-блоков и сборка групп `g:`.

Обработчики парсеров реализованы как обычные указатели на функции с указателем
контекста. Это совместимо с BC++ и не создаёт зависимости от современного
механизма type erasure.

## Сборка в C++Builder 6

1. Откройте `NmeaDecoderExt.bpr` в C++Builder 6.
2. При необходимости укажите путь к заголовкам DHF в настройках проекта.
3. Выполните **Project → Build All**. Результат — `NmeaDecoderExt.dll`.

Файл проекта уже содержит относительные пути к `NMEA` и
`DHF/_example/include`, а также список всех компилируемых единиц. Точка входа
DLL для BC++ определена как `DllEntryPoint`.

## CMake

`CMakeLists.txt` оставлен для генераторов Windows, но использует стандарт
C++98 и создаёт только DLL-модуль. Для сборки Windows DLL используйте,
например:

```powershell
cmake -S . -B build-borland-compatible
cmake --build build-borland-compatible --config Release
```
