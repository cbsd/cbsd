# CBSD Shell (cbsdsh) — Кастомные модификации интерпретатора dash

## Обзор проекта

**CBSD** — это фреймворк для управления виртуальными окружениями (jails, bhyve, Xen и др.)
в FreeBSD и Linux. В качестве интерпретатора командных скриптов CBSD использует
собственную модифицированную версию **dash** (Debian Almquist Shell) — POSIX-совместимого
шелла, изначально созданного Кеннетом Алмквистом (Kenneth Almquist).

Модифицированный исходный код интерпретатора расположен в директории:
`/home/oleg/cbsdsh/src`

Исполняемый файл шелла устанавливается как `/usr/local/bin/cbsd`, и скрипты CBSD
используют shebang `#!/usr/local/bin/cbsd`.

Все кастомные builtin-команды зарегистрированы в файле `builtins.def.in` и
добавлены в секции `// CIX`.

---

## Архитектура кастомных builtin-команд

Встроенные команды CBSD делятся на следующие функциональные группы:

1. **SQLite-интеграция** — выполнение SQL-запросов прямо из шелла
2. **JSON-обработка** — встроенный jq-процессор
3. **Работа со строками и числами** — substr, strpos, strlen, roundup, is_number
4. **Информационные команды** — about, version
5. **Мониторинг файловых систем** — cbsd_fwatch (inotify/kqueue)
6. **Мониторинг процессов** — cbsd_pwait
7. **Логирование** — cbsdlogger
8. **Управление задачами** — spawntask
9. **Управление jail** — cbsdjls
10. **Конфигурация** — updateconf, cixinit, cix_read_dir
11. **Перехват вывода** — capture
12. **Время** — gettime, update_idle

---

## Матрица: builtin-команда → исходный файл

| Builtin-команда    | Символ в C         | Исходный файл                     | Платформа         |
|--------------------|--------------------|------------------------------------|-------------------|
| cbsdsqlro          | sqlitecmdro        | src/sqlcmd.c                       | Все               |
| cbsdsqlrw          | sqlitecmdrw        | src/sqlcmd.c                       | Все               |
| cbsdsqlro_vars     | sqlitecmdro_vars   | src/sqlcmd.c                       | Все               |
| cbsdsqlquery       | sqlitecmdquery     | src/sqlcmd.c                       | Все               |
| jq                 | jqcmd              | src/jqcmd.c (sqlcmd.c — заголовок) | Все               |
| about              | aboutcmd           | src/about.c                        | Все               |
| version            | versioncmd         | src/about.c                        | Все               |
| is_number          | is_numbercmd       | src/mystring.c                     | Все               |
| substr             | substrcmd          | src/mystring.c                     | Все               |
| strpos             | strposcmd          | src/mystring.c                     | Все               |
| roundup            | roundupcmd         | src/mystring.c                     | Все               |
| strlen             | strlencmd          | src/mystring.c                     | Все               |
| updateconf         | updateconf         | src/mystring.c                     | Все               |
| gettime            | gettimecmd         | src/mystring.c                     | Все               |
| cbsd_fwatch        | cbsd_fwatchcmd     | src/cbsd_fwatch.c                  | FreeBSD (kqueue)  |
| cbsd_fwatch        | cbsd_fwatchcmd     | src/cbsd_fwatch_linux.c            | Linux (inotify)   |
| spawntask          | spawncmd           | src/spawn_task.c                   | Все               |
| cbsdjls            | cbsdjlscmd         | src/jail.c                         | FreeBSD           |
| cbsdjls            | cbsdjlscmd         | src/cbsd_fwatch.c                  | DragonFly/NetBSD  |
| cbsdjls            | cbsdjlscmd         | src/cbsd_fwatch_linux.c            | Linux (noop)      |
| cbsdlogger         | cbsdloggercmd      | src/logger.c                       | Все               |
| cbsd_pwait         | cbsd_pwaitcmd      | src/cbsd_pwait.c                   | FreeBSD (kqueue)  |
| cbsd_pwait         | cbsd_pwaitcmd      | src/cbsd_pwait_linux.c             | Linux (epoll)     |
| cixinit            | cixinitcmd         | src/main.c                         | Все               |
| cix_read_dir       | cix_read_dir       | src/expand.c                       | Все               |
| capture            | capturecmd         | src/eval.c                         | Все               |
| update_idle        | update_idlecmd     | src/sqlcmd.c                       | Все               |

---

## Подробное описание каждой builtin-команды

### 1. cbsdsqlro — SQLite read-only запрос

**Файл:** `src/sqlcmd.c`
**Функция:** `sqlitecmdro()`
**Синтаксис:** `cbsdsqlro <dbfile> <SQL-запрос>`

Выполняет SELECT-запрос к SQLite-базе данных в режиме только для чтения.
Результат выводится в stdout, значения разделены разделителем `sqldelimer`
(по умолчанию `|`, задаётся через переменную окружения).

**Ключевые особенности:**
- Открывает базу в режиме `SQLITE_OPEN_READONLY`
- Выполняется в дочернем процессе (fork) для изоляции от основного шелла
- Если путь к БД не абсолютный, берётся из переменной `dbdir` + суффикс `.sqlite`
- Busy timeout: 25 секунд
- Кастомный разделитель через переменную `sqldelimer`
- Retry до 50 раз при `SQLITE_BUSY`

**Пример:**
```sh
cbsdsqlro local "SELECT jname,jid FROM jails"
```

---

### 2. cbsdsqlrw — SQLite read-write запрос

**Файл:** `src/sqlcmd.c`
**Функция:** `sqlitecmdrw()`
**Синтаксис:** `cbsdsqlrw <dbfile> <SQL-запрос>`

Выполняет произвольный SQL-запрос с возможностью записи (INSERT, UPDATE, DELETE, CREATE).
Запрос оборачивается в транзакцию (BEGIN TRANSACTION ... COMMIT), при ошибке выполняется
ROLLBACK.

**Ключевые особенности:**
- Открывает базу в режиме `SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE`
- PRAGMA mmap_size = 209715200
- PRAGMA journal_mode = WAL
- PRAGMA synchronous = NORMAL
- Выполнение в дочернем процессе (fork)
- Автоматическая транзакция

**Пример:**
```sh
cbsdsqlrw local "INSERT INTO jails (jname,ip) VALUES ('testjail','10.0.0.1')"
```

---

### 3. cbsdsqlro_vars — SQLite запрос с присвоением результатов в переменные шелла

**Файл:** `src/sqlcmd.c`
**Функция:** `sqlitecmdro_vars()`
**Синтаксис:** `cbsdsqlro_vars <dbfile> <SQL-запрос> [var1 var2 ...]`

Выполняет read-only SQL-запрос и присваивает значения столбцов в shell-переменные.
Если запрос возвращает несколько строки, значения конкатенируются через `\n`.

**Ключевые особенности:**
- Число заданных переменных может быть меньше числа столбцов — тогда для остальных
  используются имена столбцов
- Результат передаётся из дочернего процесса через pipe в бинарном формате:
  `uint32 count, (uint32 nlen, uint32 vlen, bytes...)`
- При отсутствии строк все указанные переменные устанавливаются в ""

**ИЗВЕСТНЫЙ БАГ: не использовать для multi-row запросов!**

Если запрос возвращает **несколько строк**, `cbsdsqlro_vars` может вызвать порчу переменных
в shell (например, `Illegal number` ошибки с именами столбцов SQL-таблиц).
Причина: `setvar()` в dash вызывается из parent-процесса после `fork()` + pipe,
и embedded `\n` в значениях приводит к corruption переменной или соседних переменных
в хеш-таблице shell'а.

**Ниже приведён минимальный воспроизводимый пример бага:**
```sh
# Безопасно (single-row, LIMIT 1):
cbsdsqlro_vars local.sqlite "SELECT type FROM controllers WHERE name='virtio'" _type

# НЕБЕЗОПАСНО (multi-row, может corruptить переменные):
cbsdsqlro_vars local.sqlite "SELECT name FROM controllers WHERE enabled!='0'" _list
```

**Рекомендуемые альтернативы для multi-row:**
```sh
# Вариант 1: capture (builtin, без fork/subshell):
capture _list cbsdsqlro local.sqlite "SELECT name FROM controllers WHERE enabled!='0'"
for _name in ${_list}; do ... done

# Вариант 2: subshell + xargs (проверено, работает):
_list=$( cbsdsqlro local.sqlite "SELECT name FROM controllers WHERE enabled!='0'" | ${XARGS_CMD} )
for _name in ${_list}; do ... done
```

**Примеры:**
```sh
# Имена переменных заданы явно (a, b):
cbsdsqlro_vars /usr/jails/var/db/local.sqlite "SELECT jname,ip4_addr FROM jails LIMIT 1" a b
echo $a
echo $b

# Если имена переменных не указаны — используются имена столбцов:
cbsdsqlro_vars /usr/jails/var/db/local.sqlite "SELECT jname,ip4_addr FROM jails LIMIT 1"
echo $jname
echo $ip4_addr

# В контексте скрипта с подстановкой:
cbsdsqlro_vars "${_sqlfile}" "SELECT jid,jname FROM bhyve WHERE jname='$1'" myjid myjname
echo "JID=$myjid NAME=$myjname"
```

---

### 4. cbsdsqlquery — SQLite запрос с JSON-выводом

**Файл:** `src/sqlcmd.c`
**Функция:** `sqlitecmdquery()`
**Синтаксис:** `cbsdsqlquery <dbfile> <SQL-запрос>`

Выполняет read-only SQL-запрос и выводит каждую строку результата как отдельный JSON-объект.
Использует библиотеку jq/jv для формирования JSON.

**Ключевые особенности:**
- Автоматическое определение типов: INTEGER → number, FLOAT → double, TEXT → string,
  NULL → null, BLOB → null
- Одна строка = один JSON-объект на строку stdout
- Открытие БД в режиме READONLY с SHAREDCACHE

**Примеры:**
```sh
# Фильтрация через jq:
cbsdsqlquery local "SELECT jname,ip FROM jails" | jq '.jname'

# Использование ${VAR.field} для обращения к полям JSON-строки:
# Каждая строка результата — это JSON-объект, из которого можно достать поля
# конструкции ${j@.fieldname}:
cbsdsqlquery ${workdir}/var/db/local.sqlite "SELECT * FROM jails" | while read j; do
        printf "%6s %-20s %s
" ${j@.jid} ${j@.jname} ${j@.host_hostname}
done
```

---

### 5. jq — встроенный JSON-процессор

**Файл:** `src/jqcmd.c`
**Функция:** `jqcmd()`
**Синтаксис:** `jq [опции] <фильтр> [файл...]`

Полноценная интеграция библиотеки jq (libjq) прямо в шелл. Позволяет обрабатывать
JSON-данные без вызова внешней команды jq.

**Поддерживаемые опции:**
- `--slurp` / `-s` — объединить все JSON-объекты в массив
- `--raw-input` / `-R` — считать входные данные как строки
- `--raw-output` / `-r` — выводить без кавычек
- `--raw-output0` — выводить без кавычек с разделителем NUL
- `--ascii-output` / `-a` — выводить только ASCII-экранирование
- `--color-output` / `-C` — цветной вывод
- `--monochrome-output` / `-M` — монохромный вывод
- `--sort-keys` / `-S` — сортировать ключи объекта
- `--exit-status` / `-e` — возвращать ненулевой статус при false/null
- `--from-file` / `-f` — читать фильтр из файла
- `--argjson` — передать JSON-значение как переменную
- `--arg` — передать строку как переменную
- `--args` / `--jsonargs` — передать аргументы

**Примеры:**
```sh
echo '{"name":"test","id":1}' | jq '.name'
jq '.users[].name' data.json
echo '{}' | jq --arg v "hello" '.msg = $v'
```

---

### 6. about — информация о проекте

**Файл:** `src/about.c`
**Функция:** `aboutcmd()`
**Синтаксис:** `about`

Выводит строку с названием и версией проекта: `CBSD Project. Version X.Y.Z`

**Пример:**
```sh
about
# Вывод: CBSD Project. Version 15.1.0a
```

---

### 7. version — версия CBSD

**Файл:** `src/about.c`
**Функция:** `versioncmd()`
**Синтаксис:** `version`

Выводит только номер версии без дополнительного текста.

**Пример:**
```sh
ver=$(version)
```

---

### 8. is_number — проверка числового значения

**Файл:** `src/mystring.c`
**Функция:** `is_numbercmd()` (обёртка) → `is_number()` (внутренняя)
**Синтаксис:** `is_number <строка>`

Возвращает 0 (true), если строка состоит только из цифр, иначе 1 (false).
Полезна для валидации ввода в скриптах.

**Примеры:**
```sh
if is_number "$input"; then
    echo "Это число"
fi
```

---

### 9. substr — извлечение подстроки

**Файл:** `src/mystring.c`
**Функция:** `substrcmd()`
**Синтаксис:** `substr --pos=N --len=M --str=STRING`

Извлекает подстроку из строки.

**Параметры:**
- `--pos` — начальная позиция (счёт от 1)
- `--len` — длина (0 = до конца строки)
- `--str` — исходная строка

**Примеры:**
```sh
substr --pos=1 --len=3 --str="Hello World"
# Вывод: Hel

substr --pos=7 --len=0 --str="Hello World"
# Вывод: World
```

---

### 10. strpos — поиск позиции подстроки

**Файл:** `src/mystring.c`
**Функция:** `strposcmd()`
**Синтаксис:** `strpos --search=SUBSTRING --str=STRING`

Находит позицию первого вхождения подстроки. Возвращает 0-based индекс
или 0, если подстрока не найдена. (Внимание: 0 возвращается и при отсутствии
совпадения, и при совпадении на позиции 0.)

**Примеры:**
```sh
strpos --search="World" --str="Hello World"
# Вывод: 6
```

---

### 11. roundup — округление вверх до кратности

**Файл:** `src/mystring.c`
**Функция:** `roundupcmd()`
**Синтаксис:** `roundup --num=N --multiple=M`

Округляет число N вверх до ближайшего кратного M.

**Примеры:**
```sh
roundup --num=1477 --multiple=500
# Вывод: 1500

roundup --num=1000 --multiple=500
# Вывод: 1000
```

---

### 12. strlen — длина строки

**Файл:** `src/mystring.c`
**Функция:** `strlencmd()`
**Синтаксис:** `strlen <строка>`

Выводит длину строки в символах (без перевода строки).

**Примеры:**
```sh
len=$(strlen "Hello")
echo $len
# Вывод: 5
```

---

### 13. updateconf — обновление конфигурационного файла

**Файл:** `src/mystring.c`
**Функция:** `updateconf()`
**Синтаксис:** `updateconf <путь_к_файлу> key=val [key2=val2 ...]`

Атомарно обновляет значения в конфигурационном файле в формате `KEY="VALUE"`.

**Ключевые особенности:**
- File locking с таймаутом 2 секунды (flock)
- Синтаксическая проверка обновлённого файла через встроенный парсер dash (`-n`)
- Валидация синтаксиса шелла ДО замены оригинального файла
- Поддержка += для добавления к существующему значению
- Корректная обработка кавычек и экранирование `"`, `\`
- Сохранение прав доступа (mode, uid, gid) оригинального файла
- Новые ключи добавляются в конец файла
- При ошибке синтаксиса файл НЕ заменяется (rollback)

**Примеры:**
```sh
updateconf /etc/cbsd.conf myvar="new_value"
updateconf /etc/cbsd.conf pkglist+=" additional_pkg"
```

---

### 14. gettime — текущее время в epoch-секундах

**Файл:** `src/mystring.c`
**Функция:** `gettimecmd()`
**Синтаксис:** `gettime <ИМЯ_ПЕРЕМЕННОЙ>`

Присваивает указанной переменной текущее время в секундах Unix-эпохи (аналог `date +%s`).
Использует `gettimeofday()`,避免 вызов внешних команд.

**Примеры:**
```sh
gettime start_ts
# ... длительная операция ...
gettime end_ts
elapsed=$((end_ts - start_ts))
echo "Заняло ${elapsed} секунд"
```

---

### 15. cbsd_fwatch — мониторинг изменений файла

**Файл:** `src/cbsd_fwatch.c` (FreeBSD, kqueue) / `src/cbsd_fwatch_linux.c` (Linux, inotify)
**Функция:** `cbsd_fwatchcmd()`
**Синтаксис:** `cbsd_fwatch --file=PATH --timeout=N`

Ожидает модификации указанного файла. Время ожидания задаётся в секундах (0 = бесконечно).

**Платформенные реализации:**
- **FreeBSD/BSD:** использует `kqueue()` + `EVFILT_VNODE`. Отслеживает: NOTE_DELETE,
  NOTE_WRITE, NOTE_EXTEND, NOTE_ATTRIB, NOTE_LINK, NOTE_RENAME, NOTE_REVOKE
- **Linux:** использует `inotify_init1()` + `poll()`. Отслеживает: IN_OPEN, IN_CLOSE,
  IN_MODIFY, IN_DELETE, IN_MOVE_SELF и др.

**Вывод:** строка с типом события (deleted, written, extended, renamed и т.д.)

**Примеры:**
```sh
cbsd_fwatch --file=/tmp/mylock --timeout=30
```

---

### 16. spawntask — запуск фоновой задачи

**Файл:** `src/spawn_task.c`
**Функция:** `spawncmd()`
**Синтаксис:** `spawntask <jobid> <logfile> <cmd...>`

Запускает команду в фоновом дочернем процессе с перенаправлением stdout/stderr
в лог-файл. Дочерний процесс выполняется через `/usr/local/bin/cbsd -c`.

**Ключевые особенности:**
- stdin перенаправлен из `/dev/null`
- stdout и stderr пишутся в указанный лог-файл (режим truncate)
- Процесс запускается в новой сессии (`setsid()`)
- Родительский процесс ожидает завершения дочернего (`waitpid`)
- Окружение: PATH, NOCOLOR=1, INTER=0, workdir из shell-переменной

**Примеры:**
```sh
spawntask 42 /var/log/cbsd/task.log long_running_command arg1 arg2
```

---

### 17. cbsdjls — список jail

**Файл:** `src/jail.c` (FreeBSD/DragonFly) / `src/cbsd_fwatch.c` (DragonFly/NetBSD noop) /
       `src/cbsd_fwatch_linux.c` (Linux noop)
**Функция:** `cbsdjlscmd()`
**Синтаксис:** `cbsdjls [-q]`

Выводит список запущенных jail с их JID и именем.

**Реализации по платформам:**
- **FreeBSD:** использует системный вызов `jailparam_get()` из `<jail.h>`.
  Без флагов выводит: `JID jailname`. С флагом `-q` — только JID.
- **DragonFly:** использует `sysctlbyname("jail.list")`, парсит вывод и
  форматирует как `JID jailname`.
- **Linux/NetBSD:** заглушка (noop), возвращает 0.

**Примеры:**
```sh
cbsdjls          # "1 myjail\n2 webserver"
cbsdjls -q       # "1\n2"
```

---

### 18. cbsdlogger — системное логирование

**Файл:** `src/logger.c`
**Функция:** `cbsdloggercmd()`
**Синтаксис:** `cbsdlogger [DEBUG|VERBOSE|NOTICE|WARNING] сообщение...`

Записывает сообщение в лог-файл и/или syslog.

**Ключевые особенности:**
- 4 уровня логирования: DEBUG (0), VERBOSE (1), NOTICE (2), WARNING (3)
- Конфигурируется через shell-переменные:
  - `CBSD_SYSLOG_ENABLED` — включить syslog (1/0)
  - `CBSD_SYSLOG_VERBOSITY` — минимальный уровень
  - `CBSD_LOGFILE` — путь к лог-файлу
- Максимальная длина сообщения: 1024 байта (LOG_MAX_LEN)
- Длинные сообщения обрезаются с суффиксом `...`
- Формат записи в файл: `PID:DD Mon HH:MM:SS.mmm [LEVEL] message`

**Примеры:**
```sh
cbsdlogger NOTICE "Задача завершена успешно"
cbsdlogger WARNING "Высокое потребление памяти"
```

---

### 19. cbsd_pwait — ожидание завершения процесса

**Файл:** `src/cbsd_pwait.c` (FreeBSD, kqueue) / `src/cbsd_pwait_linux.c` (Linux, epoll+signalfd)
**Функция:** `cbsd_pwaitcmd()`
**Синтаксис:** `cbsd_pwait --pid=PID --timeout=N`

Ожидает завершения указанного процесса. Возвращает 0 при завершении,
1 при таймауте.

**Платформенные реализации:**
- **FreeBSD/BSD:** `kqueue()` + `EVFILT_PROC` с `NOTE_EXIT`. Блокируется до
  завершения процесса или таймаута.
- **Linux:** `epoll` + `signalfd(SIGCHLD)` + `timerfd`. Предварительно проверяет
  существование процесса через `kill(pid, 0)`.

**Примеры:**
```sh
some_command &
bg_pid=$!
cbsd_pwait --pid=$bg_pid --timeout=60
```

---

### 20. cixinit — инициализация параметров CIX-скрипта

**Файл:** `src/main.c`
**Функция:** `cixinitcmd()`
**Синтаксис:** `cixinit`

Мощная команда для инициализации параметров CBSD-скриптов из аргументов командной строки.
Обеспечивает framework для парсинга `key=value` аргументов, валидации обязательных
параметров и встраиваемой документации.

**Операции (в порядке выполнения):**

1. `--help` без ADDHELP → запуск `${progname} help`
2. `--help` с ADDHELP → вывод содержимого ADDHELP (с escape-последовательностями)
3. `--desc` → вывод MYDESC (описание модуля). Уважает CBSDMODULE/CBSDMODULEONLY
4. `--argstype` / `--cixargs` → вывод CIXARG/CIXOPTARG (аргументы с типами)
5. Копирование CIXARG/CIXOPTARG → MYARG/MYOPTARG (с удалением `:type` суффиксов)
6. `--args` → вывод MYARG/MYOPTARG
7. Параметры после `where` → CIXINIT_SQL_CONDITION
8. Парсинг `key=value` из аргументов → установка shell-переменных
9. Валидация: каждый MYARG параметр должен быть непустым
10. Дублирование в `o`-префикс переменные (varname → ovarname)
11. CIX_INIT_SAVE2FILE → сохранение параметров в конф. файл
12. CIX_INIT_SKIP → пропуск инициализации указанных параметров
13. Неизвестные аргументы → CIX_OTHER_ARGS

**Связанные переменные окружения:**
- `ADDHELP` — справочный текст
- `MYDESC` — описание модуля
- `CBSDMODULE` — список модулей через запятую
- `CBSDMODULEONLY` — фильтр модуля
- `CIXARG` — обязательные аргументы (формат: `name:type ...`)
- `CIXOPTARG` — опциональные аргументы
- `CIX_INIT_SKIP` — пропускаемые параметры
- `CIX_INIT_SAVE2FILE` — путь для сохранения
- `CIX_OTHER_ARGS` — неизвестные аргументы
- `CIXINIT_SQL_CONDITION` — SQL-условие из `where`

**Пример:**
```sh
MYDESC="Управление виртуальной машиной"
CIXARG="vmname:str"
CIXOPTARG="memory:int cpu:int"
ADDHELP="Использование: start vmname=NAME [memory=512] [cpu=1]"
cixinit
```

---

### 21. cix_read_dir — чтение содержимого директории

**Файл:** `src/expand.c`
**Функция:** `cix_read_dir()`
**Синтаксис:** `cix_read_dir <директория>`

Выводит список файлов в директории (за исключением скрытых файлов, начинающихся с `.`)
через пробел. Возвращает 0 при наличии файлов, 1 при ошибке или пустой директории.

**Примеры:**
```sh
files=$(cix_read_dir /usr/local/etc/cbsd/)
for f in $files; do
    echo "Found: $f"
done
```

---

### 22. capture — перехват stdout в переменную

**Файл:** `src/eval.c`
**Функция:** `capturecmd()`
**Синтаксис:** `capture <ИМЯ_ПЕРЕМЕННОЙ> <команда [аргументы...]>`

Перехватывает stdout произвольной команды (включая builtin-команды, функции и внешние
программы) и сохраняет результат в указанную shell-переменную. Завершающие переводы
строк удаляются.

**Ключевые особенности:**
- Использует временный файл (`/tmp/cbsdsh.capture.XXXXXX`) вместо pipe для избежания
  deadlock'ов
- Поддерживает builtin-команды (CMDBUILTIN), shell-функции (CMDFUNCTION) и
  внешние команды (CMDNORMAL, через vfork+exec)
- При неудаче переменная устанавливается в ""
- Код возврата совпадает с кодом возврата команды

**Примеры:**
```sh
# Перехват внешней команды:
capture CURL_CMD which curl
echo "curl found at: $CURL_CMD"

# Перехват команды с аргументами (date +%s):
capture DT date +%s
echo "Current epoch: $DT"

# Перехват builtin-команды:
capture mydir pwd
echo "Текущая директория: $mydir"

# Перехват версии CBSD:
capture ver version
echo "Версия CBSD: $ver"

# Перехват shell-функции — функция должна быть определена до вызова capture:
testfunc()
{
        a=$(( 1 + 2 ))
        echo "${a}"
}

capture X testfunc
echo "Результат: $X"

# Перехват SQL-запроса:
capture result cbsdsqlro local "SELECT count(*) FROM jails"
echo "Количество jail: $result"
```

**ВАЖНОЕ ОГРАНИЧЕНИЕ: `capture` + `getopts`/`shift`/`OPTIND`:**

`capture` выполняет shell-функцию **в текущем процессе** (без fork/subshell).
Если вызываемая функция использует `getopts`, `shift $((OPTIND - 1))` или
иным образом модифицирует `OPTIND`, это **разрушает** состояние вызывающего
контекста: обнуляет позиционные параметры (`$1`, `$2`, ...) и портит `OPTIND`
для последующих вызовов `getopts` в родительском скоупе.

**Нельзя** использовать `capture` с функциями, содержащими `getopts` + `shift`.
Для них **обязательно** использовать `$()` (subshell), который изолирует
побочные эффекты:

```sh
# НЕПРАВИЛЬНО — getopts/shift внутри функции портят вызывающий контекст:
capture _result my_func_with_getopts -a foo -b bar

# ПРАВИЛЬНО — $() создаёт subshell, изолирующий getopts/shift:
_result=$( my_func_with_getopts -a foo -b bar )
```

**Безопасно** использовать `capture` с:
- builtin-командами: `cbsdsqlro`, `strlen`, `substr`, `which`, `cbsdsqlro_vars`
- внешними скриптами/бинарниками: `get-next-tcp-port`, `sqlcli`, `getinfo`
- простыми функциями, которые **не** используют `getopts`/`shift`: `get_vm_cores_by_topology`, `mac_gen`

**Небезопасно** использовать `capture` с:
- функциями, использующими `getopts` + `shift $(($OPTIND - 1))`: например `get_pcislot_args`

---

### 23. update_idle — обновление времени простоя ноды

**Файл:** `src/sqlcmd.c`
**Функция:** `update_idlecmd()`
**Синтаксис:** `update_idle <nodename>`

Обновляет поле `idle` в таблице `nodelist` базы `nodes` текущим временем
(`datetime('now','localtime')`). Используется для heartbeat-механизма кластера CBSD.

**Пример:**
```sh
update_idle "node1.example.com"
```

---

## Архитектура SQLite-подсистемы

SQLite-операции в CBSD Shell используют **изоляцию через fork**:

```
┌─────────────────┐         pipe          ┌─────────────────┐
│  Родительский   │ ◄──────────────────── │  Дочерний       │
│  процесс (шлл)  │    stdout → pipe      │  процесс        │
│                  │                       │  (sqlite3 ops)  │
│  Читает pipe    │                       │  sql_open()     │
│  Выводит stdout │                       │  sql_exec()     │
└─────────────────┘                       └─────────────────┘
```

Это сделано для предотвращения heap-коррупции в основном процессе шелла на
некоторых платформах и сборках libc.

**Глобальные константы SQLite:**
- `CBSD_SQLITE_BUSY_TIMEOUT` = 25000 мс
- `DEFSQLDELIMER` = `"|"` (разделитель столбцов по умолчанию)
- `DBPOSTFIX` = `".sqlite"` (суффикс файлов БД)

**Управление через shell-переменные:**
- `dbdir` — директория с БД (для относительных путей)
- `sqldelimer` — кастомный разделитель столбцов

---

## Вспомогательные функции в updateconf

Команда `updateconf` (в `src/mystring.c`) содержит ряд вспомогательных функций:

- **`parse_kv_args()`** — парсит key=value аргументы с поддержкой += и кавычек
- **`extract_existing_value_after_eq()`** — извлекает значение после = в строке конфигурации
- **`escape_for_double_quotes()`** — экранирует `\` и `"` для вставки в двойные кавычки
- **`try_lock_with_timeout()`** — блокировка файла (flock) с монотонным таймаутом
- **`dash_syntax_check_file()`** — валидация синтаксиса шелл-файла через встроенный парсер dash
- **`monotonic_ms()`** — текущее время в миллисекундах (CLOCK_MONOTONIC)

---

## Логирование (logger.c)

Встроенная система логирования CBSD:

| Уровень     | Константа   | Значение |
|-------------|-------------|----------|
| DEBUG       | LL_DEBUG    | 0        |
| VERBOSE     | LL_VERBOSE  | 1        |
| NOTICE      | LL_NOTICE   | 2        |
| WARNING     | LL_WARNING  | 3        |

Переменные окружения:
- `CBSD_SYSLOG_ENABLED` — включить отправку в syslog
- `CBSD_SYSLOG_VERBOSITY` — минимальный уровень (DEBUG, VERBOSE, NOTICE, WARNING)
- `CBSD_LOGFILE` — путь к файлу лога

---

## Связанные заголовочные файлы

| Файл           | Описание                                        |
|----------------|-------------------------------------------------|
| sqlcmd.h       | Константы SQLite, макрос ERROR_SQLITE, DBI defs |
| about.h        | Макрос VERSION                                  |
| logger.h       | Константы логирования, глобальные переменные    |
| spawn_task.h   | Перечисление log_type                           |
| mystring.h     | Прототипы строковых функций                     |

---

## Регистрация builtin-команд

Все builtin-команды CBSD зарегистрированы в файле `src/builtins.def.in` в формате:

```
символ_в_C    имя_команды
```

Секция регистрации (строки 96–121 `builtins.def.in`):

```c
// CIX
sqlitecmdro         cbsdsqlro
sqlitecmdrw         cbsdsqlrw
sqlitecmdro_vars    cbsdsqlro_vars
sqlitecmdquery      cbsdsqlquery
jqcmd               jq
aboutcmd            about
is_numbercmd        is_number
substrcmd           substr
strposcmd           strpos
roundupcmd          roundup
strlencmd           strlen
versioncmd          version
cbsd_fwatchcmd      cbsd_fwatch
spawncmd            spawntask
cbsdjlscmd          cbsdjls
cbsdloggercmd       cbsdlogger
cbsd_pwaitcmd       cbsd_pwait
update_idlecmd      update_idle
cixinitcmd          cixinit
cix_read_dir        cix_read_dir
updateconf          updateconf
// CIX
capturecmd          capture
gettimecmd          gettime
```

---

---

## Расширения синтаксиса шелла (отсутствуют в стандартном dash)

Помимо builtin-команд, в CBSD-шелл добавлены расширения синтаксиса parser'а,
которые отсутствуют в оригинальном dash и приближают его к bash/zsh.

### Here-string оператор `<<<`

**Файлы:** `src/parser.c` (NSTRING, строка ~1424), `src/redir.c` (openhere, case NSTRING)

Оператор `<<<` (here-string) передаёт строку на stdin команды без использования pipe
и echo/cat. Строка автоматически дополняется переводом строки.

**Тип узла AST:** `NSTRING`

**Внутренняя реализация:**
- Парсер при обнаружении `<<` затем ещё `<` создаёт узел `NSTRING`
- В `openhere()` строка расширяется через `expandarg()` с флагом `EXP_QUOTED`
- Данные передаются через pipe; при строках длиннее `PIPESIZE` используется fork
- К строке автоматически добавляется `\n`

**Синтаксис:**
```
команда <<< "строка"
```

**Примеры:**
```sh
# Вместо echo "Hello FreeBSD world" | grep FreeBSD
# можно использовать here-string:
variable="Hello FreeBSD world"
grep "FreeBSD" <<< "$variable"
echo $?
# Вывод: 0

# Если подстрока не найдена — возвращается ненулевой статус:
grep "FddreeBSD" <<< "$variable"
echo $?
# Вывод: 1

# Удобно для передачи переменных в stdin без порождения echo-процесса:
read line <<< "one two three"
echo $line
# Вывод: one two three
```

**Отличие от here-document (`<<`):**
```
# Here-document (многострочный, стандарт POSIX):
cat <<EOF
line1
line2
EOF

# Here-string (однострочный bash-синтаксис, добавлен в CBSD):
cat <<< "одна строка"
```

---

### C-style арифметический цикл `for ((;;))`

**Файлы:** `src/parser.c` (`read_forarith()`, строка ~909),
          `src/eval.c` (`evalforarith()`, строка ~461)

Синтаксический сахар для циклов с арифметическими выражениями, аналогичный
bash/zsh. Отсутствует в стандартном POSIX sh и оригинальном dash.

**Тип узла AST:** `nforarith` (структура с полями `init`, `cond`, `update`, `body`, `linno`)

**Внутренняя реализация:**
- Парсер (`read_forarith()`) разбирает три выражения между `((` и `))`, разделённые `;`
- Каждое выражение дублируется в malloc'd буфер (`arith_for_expr_dup()`), чтобы избежать
  проблем со стековым аллокатором
- Вычисление каждого выражения через `arith_for_eval()` → `arith()` (ARI-парсер)
- Типы `intmax_t`, поддерживаются все стандартные арифметические операторы
- Поддержка `break`/`continue` через `skiploop()`

**Синтаксис:**
```
for ((инициализация; условие; обновление)); do
    команды
done
```

**Примеры:**
```sh
# Вместо:
for i in $( jot 5 ); do
       echo "id: ${i}"
done

# Лучше (без вызова внешней команды jot, без word splitting):
for ((i=1; i<=5; i++)); do
       echo "id: ${i}"
done
# Вывод:
# id: 1
# id: 2
# id: 3
# id: 4
# id: 5

# Обратный отсчёт:
for ((i=10; i>=1; i--)); do
       echo "Обратный отсчёт: ${i}"
done

# Пустые выражения допустимы (бесконечный цикл с break):
n=0
for ((;;)); do
    n=$((n + 1))
    [ $n -ge 3 ] && break
    echo "iteration $n"
done

# Несколько переменных:
for ((i=0, j=10; i<j; i++, j--)); do
    echo "i=$i j=$j"
done
```

**Отличие от стандартного POSIX for-in:**
```
# POSIX (стандартный dash) — зависит от jot/seq, word splitting:
for i in 1 2 3 4 5; do echo $i; done

# C-style (CBSD extension) — чистая арифметика, нет внешних вызовов:
for ((i=1; i<=5; i++)); do echo $i; done
```

---

## Полная карта исходных файлов → builtin-функций

### src/sqlcmd.c
- `sqlitecmdro()` — cbsdsqlro
- `sqlitecmdrw()` — cbsdsqlrw
- `sqlitecmdro_vars()` — cbsdsqlro_vars
- `sqlitecmdquery()` — cbsdsqlquery
- `update_idlecmd()` — update_idle
- `sql_open()` — внутренняя: открытие SQLite БД
- `sql_exec()` — внутренняя: выполнение SQL-выражения
- `sqlCB()` — внутренняя: callback для вывода строк SELECT
- `build_query()` — внутренняя: сборка SQL из argv
- `fork_sql_child_stdout()` — внутренняя: fork + pipe для изоляции

### src/jqcmd.c
- `jqcmd()` — jq (встроенный JSON-процессор)

### src/about.c
- `aboutcmd()` — about
- `versioncmd()` — version

### src/mystring.c
- `is_number()` — внутренняя проверка числа
- `is_numbercmd()` — is_number
- `gettimecmd()` — gettime
- `strlencmd()` — strlen
- `substrcmd()` — substr
- `strposcmd()` — strpos
- `roundupcmd()` — roundup
- `updateconf()` — updateconf
- `parse_kv_args()` — внутренняя: парсинг key=value
- `try_lock_with_timeout()` — внутренняя: файловая блокировка
- `dash_syntax_check_file()` — внутренняя: валидация синтаксиса

### src/cbsd_fwatch.c (FreeBSD)
- `cbsd_fwatchcmd()` — cbsd_fwatch (kqueue/VNODE)
- `cbsdjlscmd()` — cbsdjls (только для DragonFly/NetBSD: noop)

### src/cbsd_fwatch_linux.c (Linux)
- `cbsd_fwatchcmd()` — cbsd_fwatch (inotify)
- `cbsdjlscmd()` — cbsdjls (Linux: noop)

### src/spawn_task.c
- `spawncmd()` — spawntask
- `set_output()` — внутренняя: настройка перенаправления вывода

### src/jail.c (FreeBSD)
- `cbsdjlscmd()` — cbsdjls (jail API)
- `add_param()` — внутренняя: добавление jail-параметра
- `sort_param()` — внутренняя: сортировка параметров
- `print_jail()` — внутренняя: вывод jail (jid + name)
- `print_jids()` — внутренняя: вывод только jid

### src/logger.c
- `cbsdloggercmd()` — cbsdlogger
- `serverLogRaw()` — внутренняя: низкоуровневая запись лога
- `init_logvars()` — внутренняя: инициализация из shell-переменных
- `cbsdlog()` — внутренняя: логирование с printf-форматом (для C-кода)

### src/cbsd_pwait.c (FreeBSD)
- `cbsd_pwaitcmd()` — cbsd_pwait (kqueue/EVFILT_PROC)

### src/cbsd_pwait_linux.c (Linux)
- `cbsd_pwaitcmd()` — cbsd_pwait (epoll + signalfd + timerfd)

### src/main.c
- `cixinitcmd()` — cixinit (полный парсер параметров CIX-скриптов)

### src/expand.c
- `cix_read_dir()` — cix_read_dir

### src/eval.c
- `capturecmd()` — capture (перехват stdout в переменную)

---

## Валидация синтаксиса скриптов CBSD

Поскольку CBSD-шелл (`cbsdsh`) является **модифицированным dash** с расширенным
синтаксисом (`<<<`, `for ((;;))`, `capture`, `cbsdsqlro_vars` и др.), стандартные
инструменты проверки синтаксиса **не подходят** для валидации CBSD-скриптов.

### Неправильно:

```sh
# Стандартный sh не знает о расширениях cbsdsh:
sh -n /usr/local/cbsd/subr/bhyve.subr       # ОШИБКА ложные срабатывания

# bash использует другой парсер и может пропустить/найти не те ошибки:
bash -n /usr/local/cbsd/subr/bhyve.subr      # НЕВАЛИДНАЯ проверка
```

### Правильно:

```sh
# Используйте сам интерпретатор cbsdsh для валидации:
cbsd -n /usr/local/cbsd/subr/bhyve.subr      # Корректная проверка синтаксиса

# Если /usr/local/bin/cbsd недоступен текущему пользователю (владелец cbsd:cbsd, mode r-xr-x---):
sudo /usr/local/bin/cbsd -n /usr/local/cbsd/subr/bhyve.subr
```

Флаг `-n` заставляет интерпретатор разобрать скрипт и проверить синтаксис
**без выполнения команд**, используя парсер, идентичный runtime-поведению.

**Когда использовать `cbsd -n`:**
- После рефакторинга CBSD-скриптов
- Перед коммитом изменений в `.subr`-файлы
- После массовых замен `sed`/`awk` в CBSD-коде
- Для CI/CD проверок синтаксиса CBSD-скриптов

---

## Метаданные документа

- **Проект:** CBSD Project (https://www.bsdstore.ru)
- **Исходный код шелла:** `/home/oleg/cbsdsh/src`
- **Регистрация builtin:** `src/builtins.def.in`
- **Версия CBSD в about.h:** 15.1.0a
- **Базовый интерпретатор:** dash (Debian Almquist Shell), Herbert Xu / Kenneth Almquist
- **Лицензия:** BSD 3-clause (оригинальный dash), CBSD — BSD 2-clause
- **Платформы:** FreeBSD, DragonFlyBSD, NetBSD, Linux
- **Формат документа:** RAG-compatible Markdown (структурированные заголовки,
  таблицы, код-блоки, чёткая иерархия для retrieval-augmented generation)
