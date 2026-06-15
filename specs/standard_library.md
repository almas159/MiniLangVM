# A.3.15 Standard library

Уровень: 3  
Баллы: 20

В проекте реализована простая стандартная библиотека на самом языке MiniLangVM.

Библиотека лежит в папке `stdlib/`.

Файлы:

    stdlib/prelude.ml
    stdlib/io.ml
    stdlib/math.ml
    stdlib/string.ml
    stdlib/array.ml
    stdlib/memory.ml

Подключение выполняется флагом:

    --use-stdlib

Пример:

    ./build/minilang examples/dop_a3_15_stdlib.ml --use-stdlib

Важно: стандартная библиотека написана не как C++ builtins, а как обычные функции на MiniLangVM.

При использовании `--use-stdlib` компилятор добавляет stdlib-файлы к пользовательской программе, после чего весь объединённый исходный код проходит обычные стадии:

    lexer
    parser
    AST
    semantic analyzer
    bytecode generator
    VM

функции стандартной библиотеки используют префиксы:

    math_
    string_
    array_
    io_
    memory_
    std_

Примеры функций:

    math_abs
    math_max
    math_min
    math_clamp
    math_square
    math_is_even
    string_repeat2
    string_repeat3
    array_sum_int3
    array_max_int3
    io_println_string
    memory_clone
