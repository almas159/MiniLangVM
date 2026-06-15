# MiniLangVM — отчёт по дополнительным возможностям

MiniLangVM — C-подобный язык программирования с собственным компиляторным pipeline:

```
исходный код .ml
-> lexer
-> parser
-> AST
-> semantic analyzer
-> bytecode generator
-> VM
```

Основной backend проекта — байткодная виртуальная машина. Дополнительно есть x86-64 NASM backend.


# Таблица допов

| Возможность                              | Номер допа | Уровень | Баллы | 
| Автоматический вывод типа через `var`    |      A.1.7 |       1 |     5 |                                    |
| Битовые операции                         |      A.1.9 |       1 |     5 | `*=`, `/=`, `%=` и битовые варианты 
| Метафункции `sizeof`, `typeid`, `typeof` |     A.1.13 |       1 |     5 |  
| Блочные комментарии                      |     A.1.15 |       1 |     5 |есть вложенность `/* /* */ */'
| Generic-функции                          |     A.2.16 |       2 |    10 | 
| Автовывод generic type arguments         |      A.3.7 |       3 |    20 | Работает для generic-функций                                                           |
| Перегрузка функций                       |      A.2.8 |       2 |    10 | 
| Перегрузка с неявными приведениями       |      A.3.1 |       3 |    20 | Расширяет A.2.8     
| Bytecode VM                              |      B.2.5 |       2 |    10 | Основной backend проекта                                                               |
| Дополнительные CLI-режимы                |      B.1.8 |       1 |     5 | `--emit-asm`, `--time-phases`, `--dump-bytecode`                                       |
| Golden tests                             |     B.1.10 |       1 |     5 | 
| Standard library на языке                |     A.3.15 |       3 |    20 | 

# A.1.7 — автоматический вывод типов

Уровень: 1
Баллы: 5

В проекте есть `var`.

Пример:

```
var x = 10;
var y = 2.5;
var s = "hello";
var b = true;
```

Компилятор сам выводит тип по initializer:

```
x -> int
y -> float64
s -> string
b -> bool
```

После semantic analysis переменная уже имеет обычный статический тип.

То есть так нельзя:

```
var x = 10;
x = "hello";
```

Потому что `x` уже стал `int`.


---

# A.1.9 — расширенные операторы

Уровень: 1
Баллы: 5


В проекте есть битовые операции:

```
&
|
^
~
<<
>>
```

Они работают для `int` и `uint`.

Пример:

```
int a = 6;       // 110
int b = 3;       // 011

int x = a & b;   // 2
int y = a | b;   // 7
int z = a ^ b;   // 5
int l = 1 << 4;  // 16
int r = 16 >> 2; // 4
```

Для `uint` используется беззнаковая логика.

Для сдвигов есть runtime-проверка:

```
если n < 0 или n >= 32
-> runtime error: invalid shift count
```


# A.1.13 — простые встроенные метафункции

Уровень: 1
Баллы: 5

В языке есть встроенные meta-функции:

```
sizeof(expr)
typeid(expr)
typeof(expr)
```

`sizeof` возвращает `int`.

`typeid` и `typeof` возвращают строку с каноническим именем типа.

Пример:

```
sizeof(42)        // 4
typeid(42)        // "int"
typeof("hello")   // "string"
```

Эти функции распознаются semantic analyzer как специальные встроенные функции.

Codegen может сразу сгенерировать готовое значение:

```
sizeof(42)      -> PushInt 4
typeid(42)      -> PushString "int"
typeof("hello") -> PushString "string"
```



# A.1.15 — блочные комментарии

Уровень: 1
Баллы: 5


В проекте есть блочные комментарии:

```
/*
    comment
*/
```

Лексер пропускает их и не передаёт дальше в parser.

Но по ТЗ для A.1.15 может требоваться вложенность:

```
/*
    first
    /*
        second
    */
*/
```


# A.2.16 — обобщённые типы

Уровень: 2
Баллы: 10

В проекте есть generic-функции.

Пример:

```
T id<T>(T x) {
    return x;
}
```

Вызов с явным типом:

```
int a = id<int>(42);
string s = id<string>("hello");
```

Semantic analyzer сопоставляет generic-параметр `T` с реальным типом аргумента.

Например:

```
id<int>(42)
```

означает:

```
T = int
```

реализация ближе к type erasure, а не к полноценной monomorphization.

То есть semantic analyzer проверяет типы, но отдельные копии функции под каждый тип не создаются.


# A.3.7 — автовывод generic type arguments

Уровень: 3
Баллы: 20


A.3.7 расширяет A.2.16.

Идея: generic-функцию можно вызвать без явного указания типа.

Например вместо:

```
id<int>(42)
```

можно написать:

```
id(42)
```

Компилятор сам понимает:

```
T = int
```

Пример:

```
T id<T>(T x) {
    return x;
}

int main() {
    int a = id(42);
    string s = id("hello");

    return 0;
}
```

В первом вызове:

```
T = int
```

Во втором:

```
T = string
```

# A.2.8 — перегрузка функций

Уровень: 2
Баллы: 10

В проекте есть перегрузка функций.

Можно объявить несколько функций с одним именем, если у них разные типы параметров:

```
int describe(int x) {
    return 1;
}

int describe(string x) {
    return 2;
}
```

Вызов:

```
describe(10)
```

выберет функцию:

```
describe(int)
```

Вызов:

```
describe("hello")
```

выберет функцию:

```
describe(string)
```

Если две функции имеют одинаковую сигнатуру, это ошибка:

```
int f(int x) {
    return 1;
}

int f(int x) {
    return 2;
}
```

Semantic analyzer хранит список перегрузок и выбирает подходящую по типам аргументов.

После выбора он записывает в AST конкретный `functionIndex`.

Поэтому codegen уже не решает перегрузку заново.


# A.3.1 — перегрузка с неявными приведениями

Уровень: 3
Баллы: 20

A.3.1 расширяет обычную перегрузку A.2.8.

Если точной перегрузки нет, compiler пытается найти перегрузку, куда аргументы можно безопасно привести.


Пример:

```
int onlyFloat(float64 x) {
    return 10;
}

int main() {
    return onlyFloat(42);
}
```

Тут `42` имеет тип `int`.

Точной функции:

```
onlyFloat(int)
```

нет.

Но есть:

```
onlyFloat(float64)
```

Компилятор выбирает её и вставляет приведение `int -> float64`.

Важное правило:

```
точное совпадение важнее приведения
```

Пример:

```
int f(int x) {
    return 1;
}

int f(float64 x) {
    return 2;
}

int main() {
    return f(42);
}
```

Выберется `f(int)`, а не `f(float64)`.


# B.2.5 — байткодная виртуальная машина

Уровень: 2
Баллы: 10

Основной backend проекта — bytecode + VM.

Компилятор после semantic analysis генерирует собственный байткод.

VM исполняет этот байткод как стековая машина.

Пример идеи:

```
PushInt 2
PushInt 3
Add
Return
```

Операнды кладутся на стек.

Инструкция снимает значения со стека, выполняет операцию и кладёт результат обратно.

VM хранит:

```
stack       — стек значений
frames      — стек вызовов функций
locals      — локальные переменные текущего фрейма
ip          — instruction pointer
bytecode    — скомпилированная программа
```

Runtime Value хранит тип и данные:

```
int
uint
float64
bool
string
array
struct
void
```



# B.1.8 — дополнительные режимы CLI

Уровень: 1
Баллы: 5

В компиляторе есть дополнительные CLI-флаги:

```
--dump-tokens
--dump-ast
--dump-bytecode
--emit-asm
--no-run
--time-phases
--help
```

Особенно важные для B.1.8:

```
--emit-asm
--time-phases
--dump-bytecode
```

Примеры:

```
./build/minilang examples/hello.ml --dump-tokens

./build/minilang examples/hello.ml --dump-ast

./build/minilang examples/hello.ml --dump-bytecode

./build/minilang examples/hello.ml --time-phases

./build/minilang examples/hello.ml --emit-asm --no-run
```



# B.1.10 — golden tests

Уровень: 1
Баллы: 5


Golden tests — это тесты, где сравниваются:

```
stdout
stderr
exit code
```

То есть тест проверяет не только успешность запуска, но и точный вывод программы.

Пример структуры:

```
test.ml
test.stdout
test.stderr
test.code
```

Скрипт запускает программу и сравнивает фактический результат с эталоном.

Пример запуска:

```
./scripts/run_golden_tests.py --minilang ./build/minilang
```

Или через CMake:

```
make golden

ctest --output-on-failure
```


# A.3.15 — стандартная библиотека на языке

Уровень: 3
Баллы: 20


```
./build/minilang examples/dop_a3_15_stdlib.ml --use-stdlib
```


Идея A.3.15:

```
стандартная библиотека написана на самом MiniLangVM
```

Например файлы:

```
stdlib/io.ml
stdlib/math.ml
stdlib/string.ml
stdlib/array.ml
stdlib/memory.ml
```


## Расширенная система встроенных типов

Есть:

```
int / int32
uint / uint32
float64
float32
bool
string
void / unit
array
struct
```


## Литералы разных систем счисления

Есть:

```
42
0x2A
0b101010
```


## Вещественные литералы и special values

Есть:

```
1.5e2
2.5E1
inf
-inf
NaN
```


## Обобщённые массивы и массивы внутри структур



A.1.13 meta-functions                 5
B.1.8 CLI modes                       5
B.1.10 golden tests                   5
B.2.5 bytecode VM                    10
A.2.8 function overloading           10
A.3.1 overload implicit conversions  20




```
A.2.16 generic functions             10
A.3.7 type argument inference        20


A.1.7 var inference                   5
A.1.9 bitwise/extended operators      5




MiniLangVM — это C-подобный язык со статической строгой типизацией. Основной backend — B.2.5, байткодная стековая VM. Из допов реализованы A.1.13 meta-functions, B.1.8 CLI modes, B.1.10 golden tests, A.2.8 function overloading, A.3.1 overloading with implicit conversions, A.2.16 generic-functions и A.3.7 type argument inference. Некоторые допы я заявляю осторожно, потому что в ТЗ они шире, чем моя реализация: A.1.7, A.1.9 и A.2.16.
