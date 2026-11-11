# Breakpad Crash Handler

Отдельное Qt-приложение (crash handler), которое принимает аварийные дампы от
защищённого клиента по именованному каналу через Breakpad `CrashGenerationServer`,
записывает минидамп и показывает окно с разбором сбоя и тремя действиями:
сохранить снимок, отправить по email, отправить разработчику.

Breakpad подключён как git-субмодуль в `3rdparty/breakpad`.

## Сборка

Требования: CMake ≥ 3.21, Qt 6 (Widgets + Network + LinguistTools), MSVC
(или совместимый компилятор). C++23.

```powershell
# 1. Выкачать субмодуль (Breakpad)
git submodule update --init --recursive

# 2. Загрузить окружение MSVC (чтобы cl.exe был в PATH)
& "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64

# 3. Сконфигурировать (путь к Qt берётся из Qt6_ROOT в cmake/setup_qt.cmake,
#    можно переопределить переменной окружения Qt6_ROOT)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 4. Собрать
cmake --build build
```

Результат — `build/crash_handler.exe`. Он **не** должен проходить автозащиту
Guardant (см. план симбиоза): это отдельный незащищённый процесс, который
сам пишет дамп через `MiniDumpWriteDump`.

Для запуска вне сборочной машины разверните Qt DLL рядом с exe:

```powershell
& "C:\Work\libraries\Qt\6.10.2\msvc2022_64\bin\windeployqt.exe" --release --no-translations build\crash_handler.exe
```

(переводы уже встроены в exe как ресурс, поэтому `--no-translations`).

## Запуск

```powershell
crash_handler.exe --pipe=\\.\pipe\myapp_crash_123 --dumps=C:\dumps --app-name=MyApp
```

Аргументы:

| Аргумент    | Назначение                              |
|-------------|-----------------------------------------|
| `--pipe`    | Имя именованного канала клиента         |
| `--dumps`   | Каталог, куда писать минидампы          |
| `--parent`  | PID защищённого клиента (инф.)          |
| `--app-name`| Отображаемое имя приложения             |

## UI

- **Сохранить снимок…** — `QFileDialog` выбора пути, копирование дампа.
  При ошибке показывается `QMessageBox::critical`, главное окно остаётся открытым.
- **Отправить по email** — открывает почтовый клиент по умолчанию (`mailto:`).
- **Отправить разработчику** — HTTP-мультипарт POST на endpoint из конфигурации.
  Кнопка серая (`disabled`), если нет сети — состояние отслеживается через
  `QNetworkInformation`.

Обработчик запускается параллельно с основным приложением и **не показывает и не
создаёт своё окно**, пока с основным приложением что-то не случится: окно
создаётся лениво (в `main.cpp`) только при поступлении дампа или аварийном
завершении клиента.

## Локализация (Qt Linguist)

Исходный язык — английский, все пользовательские строки обёрнуты в `tr()`.
Перевод на русский лежит в `src/translations/crash_handler_ru_RU.ts`,
компилируется в `.qm` и встраивается в exe как ресурс (`:/translations`) через
`qt_add_translations`. Приложение выбирает перевод по системной локали.

Добавить/обновить язык:

```powershell
# перегенерировать .ts из исходников
lupdate src -ts src/translations/crash_handler_ru_RU.ts

# перевести в Qt Linguist, затем собрать заново (lrelease запускает CMake)
cmake --build build
```

## Структура

```
src/
  main.cpp            — точка входа, разбор аргументов, переводы
  mainwindow.{h,cpp}  — окно и три действия
  crashserver.{h,cpp} — обёртка CrashGenerationServer (сигналы)
  minidumpreader.*    — лёгкий разбор дампа (исключение + модуль)
  senders.{h,cpp}     — email (mailto) и HTTP-загрузка
  networkmonitor.*    — слежение за доступностью сети
  crashreport.h       — данные одного сбоя
  project_config.h.in — шаблон констант (генерируется CMake в project_config.h)
  stringutils.h       — мелкие строковые помощники
  pch.h               — прекомпилируемый заголовок
cmake/setup_qt.cmake  — подключение Qt (Qt6_ROOT, qt_standard_project_setup)
cmake/Breakpad.cmake  — сборка breakpad из субмодуля
```

## Настройка проекта (CMake-переменные)

Все константы, описывающие проект (email, endpoint, имя канала и т.д.), задаются
CMake-переменными `CRASH_*` в корневом `CMakeLists.txt` и вшиваются в
`project_config.h` на этапе конфигурации (`configure_file`). Переопределяются
при конфигурации:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release `
  -DCRASH_SUPPORT_EMAIL="support@mycompany.com" `
  -DCRASH_UPLOAD_URL="https://crash.mycompany.com/api/upload" `
  -DCRASH_PRODUCT_NAME="MyApp" `
  -DCRASH_PIPE_NAME="myapp_crash"
```

| Переменная                 | Значение по умолчанию                  | Назначение |
|----------------------------|----------------------------------------|------------|
| `CRASH_SUPPORT_EMAIL`      | `support@example.com`                  | Получатель «Отправить по email» |
| `CRASH_UPLOAD_URL`         | `https://crash.example.com/api/upload` | Endpoint «Отправить разработчику» |
| `CRASH_PRODUCT_NAME`       | `YourApp`                              | Имя приложения по умолчанию |
| `CRASH_PIPE_NAME`          | `crash_handler`                        | Имя канала (без префикса `\\.\pipe\`) |
| `CRASH_UPLOAD_FIELD_*`     | `product` / `report` / `dump`          | Поля multipart-формы |
