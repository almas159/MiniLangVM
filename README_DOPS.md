# MiniLangVM — допы

Это краткий файл по дополнительным возможностям проекта.

Основной проект: C-подобный язык MiniLangVM, компилятор на C++23, свой pipeline:

    source .ml
    -> lexer
    -> parser
    -> AST
    -> semantic analyzer
    -> bytecode generator
    -> VM

Основной backend — bytecode VM.  
Дополнительно есть x86-64 NASM backend.

---

# Таблица допов

| Доп | Уровень | Баллы | Что сделано | Статус |
|---|---:|---:|---|---|
| A.1.13 | 1 | 5 | `sizeof`, `typeid`, `typeof` | сделано |
| B.1.8 | 1 | 5 | CLI режимы: dump bytecode, asm, time phases | сделано |
| B.1.10 | 1 | 5 | golden tests | сделано |
| B.2.5 | 2 | 10 | bytecode VM | сделано |
| A.2.8 | 2 | 10 | function overloading | сделано |
| A.3.1 | 3 | 20 | перегрузка с implicit numeric conversions | сделано |
| A.3.15 | 3 | 20 | standard library на MiniLangVM через `--use-stdlib` | сделано, если проходит проверка |
| A.1.15 | 1 | 5 | вложенные блочные комментарии | сделано, если проходит проверка |

Итого по основному плану:

    A.1.13  =  5
    B.1.8   =  5
    B.1.10  =  5
    B.2.5   = 10
    A.2.8   = 10
    A.3.1   = 20
    A.3.15  = 20
    A.1.15  =  5

Всего:

    80 баллов

Дополнительно в проекте есть generic-функции и вывод type arguments, но я их считаю осторожно, потому что generic structs не реализованы.

---

# B.2.5 — bytecode VM

Уровень: 2  
Баллы: 10

Основной backend проекта — собственный байткод и стековая виртуальная машина.

Программа проходит такой путь:

    lexer
    parser
    AST
    semantic analyzer
    bytecode generator
    VM

VM использует:

    operand stack
    call stack
    stack frames
    local variables
    instruction pointer
    function table

Пример идеи байткода:

    PushInt 2
    PushInt 3
    Add
    Return

То есть операнды кладутся на стек, операция снимает их, считает результат и кладёт обратно.

Что сказать на защите:

    Вместо прямого исполнения AST у меня есть отдельный bytecode backend. После semantic analysis AST переводится в инструкции VM, а VM уже исполняет эти инструкции.

Проверка:

    ./build/minilang examples/big_full_project_demo.ml
    ./build/minilang examples/big_full_project_demo.ml --dump-bytecode

---

# B.1.8 — CLI modes

Уровень: 1  
Баллы: 5

В компиляторе есть дополнительные режимы командной строки:

    --dump-tokens
    --dump-ast
    --dump-bytecode
    --emit-asm
    --no-run
    --time-phases
    --help

Они нужны для отладки и демонстрации этапов компиляции.

Проверка:

    ./build/minilang examples/big_full_project_demo.ml --dump-tokens
    ./build/minilang examples/big_full_project_demo.ml --dump-ast
    ./build/minilang examples/big_full_project_demo.ml --dump-bytecode
    ./build/minilang examples/big_full_project_demo.ml --time-phases

---

# B.1.10 — golden tests

Уровень: 1  
Баллы: 5

Golden tests проверяют:

    stdout
    stderr
    exit code

То есть тест сравнивает не только факт запуска, но и точный вывод программы.

Скрипт:

    scripts/run_golden_tests.py

Проверка:

    ./scripts/run_golden_tests.py --minilang ./build/minilang

Или через CMake:

    cmake --build build --target golden

Что сказать на защите:

    Я сделал golden tests. Для теста хранится ожидаемый stdout, stderr и код возврата. Скрипт запускает minilang и сравнивает фактический результат с эталоном.

---

# A.1.13 — meta-functions

Уровень: 1  
Баллы: 5

Реализованы встроенные meta-functions:

    sizeof(expr)
    typeid(expr)
    typeof(expr)

Примеры:

    sizeof(42)        -> 4
    typeid(42)        -> "int"
    typeof("hello")   -> "string"

Они не являются обычными пользовательскими функциями.  
Semantic analyzer определяет тип выражения, а codegen генерирует готовое значение.

Пример:

    int main() {
        int x = 42;
        string s = "hello";

        print(sizeof(x), "\n");
        print(typeid(x), "\n");
        print(typeof(s), "\n");

        return 0;
    }

Что сказать на защите:

    Это доп A.1.13. Метафункции используют информацию статической системы типов и позволяют получить размер и имя типа выражения.

---

# A.2.8 — function overloading

Уровень: 2  
Баллы: 10

Реализована перегрузка функций.

Можно объявить несколько функций с одним именем, если у них разные типы параметров:

    int describe(int x) {
        return 1;
    }

    int describe(string x) {
        return 2;
    }

При вызове semantic analyzer выбирает нужную функцию по типам аргументов:

    describe(10)       -> describe(int)
    describe("hello")  -> describe(string)

Две функции с одинаковой сигнатурой запрещены:

    int f(int x) { return 1; }
    int f(int x) { return 2; }

Это semantic error.

Важно:

    Перегрузка решается на этапе semantic analysis.
    В AST сохраняется конкретный functionIndex.
    Codegen уже не ищет перегрузку заново.

Проверка:

    ./build/minilang examples/dop_a2_8_overload.ml
    ./build/minilang examples/dop_a2_8_overload_duplicate_bad.ml
    ./build/minilang examples/dop_a2_8_overload_no_match_bad.ml

---

# A.3.1 — overloading with implicit conversions

Уровень: 3  
Баллы: 20

Это расширение A.2.8.

Если точной перегрузки нет, semantic analyzer пробует implicit numeric conversions.

Поддерживаются числовые преобразования между:

    int
    uint
    float64

Примеры:

    int     -> uint
    int     -> float64
    uint    -> int
    uint    -> float64
    float64 -> int
    float64 -> uint

Важно:

    Эти преобразования используются только при выборе перегрузки.
    Обычные бинарные операции остаются строгими.
    Например int + uint без cast всё равно запрещён.

Алгоритм:

    1. сначала ищется точное совпадение;
    2. если точного нет, ищутся кандидаты с implicit conversions;
    3. если кандидат один — он выбирается;
    4. если несколько кандидатов одинаково подходят — ambiguous overload call.

Пример:

    int onlyFloat(float64 x) {
        return 10;
    }

    int main() {
        return onlyFloat(42);
    }

Здесь `42` имеет тип `int`, но функция принимает `float64`.  
Semantic analyzer выбирает `onlyFloat(float64)` и codegen вставляет приведение.

Пример неоднозначности:

    int f(int x) {
        return 1;
    }

    int f(uint x) {
        return 2;
    }

    int main() {
        return f(3.5);
    }

`3.5` можно привести и к `int`, и к `uint`, поэтому это ошибка:

    ambiguous overload call to function 'f'

Проверка:

    ./build/minilang examples/dop_a3_1_overload_implicit.ml
    ./build/minilang examples/dop_a3_1_overload_numeric_conversions.ml
    ./build/minilang examples/dop_a3_1_overload_ambiguous_bad.ml

---

# A.3.15 — standard library

Уровень: 3  
Баллы: 20

Реализована простая стандартная библиотека на самом MiniLangVM.

Файлы лежат в папке:

    stdlib/

Примерные файлы:

    stdlib/prelude.ml
    stdlib/io.ml
    stdlib/math.ml
    stdlib/string.ml
    stdlib/array.ml
    stdlib/memory.ml

Подключение:

    --use-stdlib

Пример запуска:

    ./build/minilang examples/dop_a3_15_stdlib.ml --use-stdlib

Главная идея:

    stdlib — это не C++ builtins.
    Это обычные функции на MiniLangVM.
    При --use-stdlib компилятор добавляет stdlib-файлы к пользовательской программе.
    Потом общий код проходит обычный pipeline: lexer, parser, semantic, bytecode, VM.

Так как полноценной модульной системы нет, функции называются с префиксами:

    math_
    string_
    array_
    io_
    memory_
    std_

Примеры функций:

    math_abs
    math_clamp
    math_square
    math_is_even
    string_repeat3
    string_surround
    array_sum_int3
    array_max_int3
    io_print_label_int
    memory_clone

Проверка:

    ./build/minilang --help | grep stdlib
    ./build/minilang examples/dop_a3_15_stdlib.ml --use-stdlib
    ./build/minilang examples/dop_a3_15_stdlib_bad.ml --use-stdlib

Что сказать на защите:

    Это доп A.3.15. Стандартная библиотека написана на самом MiniLangVM и подключается через --use-stdlib. После подключения её функции ничем не отличаются от обычных пользовательских функций.

---

# A.1.15 — nested block comments

Уровень: 1  
Баллы: 5

Реализованы вложенные блочные комментарии.

Обычный комментарий:

    /*
        comment
    */

Вложенный комментарий:

    /*
        outer

        /*
            inner
        */

        outer again
    */

Реализация в лексере:

    depth = 1

    если внутри встречается /*
        depth++

    если встречается */
        depth--

    комментарий закончен, когда depth == 0

Если файл закончился, а depth не равен нулю, лексер выдаёт ошибку:

    unterminated block comment

Проверка:

    ./build/minilang examples/dop_a1_15_nested_block_comments.ml
    ./build/minilang examples/dop_a1_15_nested_block_comments_bad.ml

Что сказать на защите:

    Это доп A.1.15. Лексер поддерживает вложенные блочные комментарии через счётчик глубины. Комментарии полностью игнорируются и не попадают в parser.

---

# Дополнительно: generic functions

Это можно показывать как дополнительную возможность, но я считаю осторожно.

Есть generic-функции:

    T id<T>(T x) {
        return x;
    }

Вызов с явным типом:

    id<int>(42)

Вызов с выводом типа:

    id(42)

Реализация ближе к type erasure:

    semantic analyzer проверяет типы;
    отдельные копии функций под каждый тип не создаются;
    VM работает с обычным Value.

Ограничение:

    generic structs не реализованы

Поэтому этот доп лучше заявлять осторожно.

---

# Дополнительно: ASM backend

Есть экспериментальный x86-64 NASM backend.

Запуск:

    ./build/minilang examples/asm_bitwise_int.ml --emit-asm --no-run
    nasm -f elf64 out.asm -o out.o
    gcc -no-pie out.o -o out
    ./out
    echo $?

ASM backend не покрывает весь язык.  
Основной backend — bytecode VM.

Что сказать:

    ASM backend нужен как демонстрация низкоуровневой генерации кода. Полная поддержка языка находится в VM backend.

---

# Команды полной проверки

Сборка:

    rm -rf build
    mkdir build
    cd build
    cmake ..
    make -j$(nproc)
    cd ..

Проверка перегрузок:

    ./build/minilang examples/dop_a2_8_overload.ml
    ./build/minilang examples/dop_a3_1_overload_implicit.ml
    ./build/minilang examples/dop_a3_1_overload_numeric_conversions.ml
    ./build/minilang examples/dop_a3_1_overload_ambiguous_bad.ml

Проверка stdlib:

    ./build/minilang --help | grep stdlib
    ./build/minilang examples/dop_a3_15_stdlib.ml --use-stdlib
    ./build/minilang examples/dop_a3_15_stdlib_bad.ml --use-stdlib

Проверка nested comments:

    ./build/minilang examples/dop_a1_15_nested_block_comments.ml
    ./build/minilang examples/dop_a1_15_nested_block_comments_bad.ml

Golden tests:

    ./scripts/run_golden_tests.py --minilang ./build/minilang

CTest:

    cd build
    ctest --output-on-failure
    cd ..

---

# Короткий итог для защиты

MiniLangVM — это C-подобный язык со статической строгой типизацией. Основной backend — bytecode VM. Из допов я заявляю:

    B.2.5  bytecode VM                         10
    B.1.8  CLI modes                            5
    B.1.10 golden tests                         5
    A.1.13 sizeof/typeid/typeof                 5
    A.2.8  function overloading                10
    A.3.1  overloading with implicit casts     20
    A.3.15 standard library                    20
    A.1.15 nested block comments                5

Итого:

    80 баллов

Ещё есть generic-функции и ASM backend, но generic structs не реализованы, а ASM backend является дополнительным и покрывает не весь язык.
