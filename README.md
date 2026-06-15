# MiniLangVM

MiniLangVM — это мой учебный проект по системам программирования.

Это компилятор и виртуальная машина для собственного C/Java-подобного языка программирования.

Основной путь работы такой:

```
исходный код
    -> лексер
    -> парсер
    -> AST
    -> семантический анализ
    -> генерация байткода
    -> выполнение на стековой VM
```

Главная часть проекта — это bytecode VM.

Также есть генерация x86-64 NASM ассемблера, но это дополнительная возможность. Полностью язык поддерживается именно через виртуальную машину.

---

## 1. Что умеет язык

В языке есть:

```
функции
переменные
var для вывода типа
let для неизменяемых переменных
if / else
while
for
switch / case / default
break
continue
return
пустая инструкция ;
массивы фиксированного размера
структуры
type aliases
namespace
строки
int / int32
uint / uint32
float64
float32 как alias
bool
void / unit
generic-функции
автовывод generic-типов
перегрузка функций
перегрузка с неявными приведениями
sizeof
typeid
typeof
битовые операции
print
input
len
assert
exit
panic
```

---

## 2. Структура проекта

Примерная структура:

```
MiniLangVM/
├── include/        заголовочные файлы
├── src/            исходный код компилятора и VM
├── specs/          спецификация языка
├── examples/       примеры программ
├── tests/          тесты
├── scripts/        скрипты
├── CMakeLists.txt
├── README.md
└── report.md
```

В `include` лежат `.hpp` файлы.

В `src` лежит основная реализация.

В `examples` лежат программы на моём языке.

В `specs` лежит описание грамматики, типов, семантики и генерации кода.

---

## 3. Что нужно для сборки

Нужно:

```
C++23 компилятор
CMake
make или ninja
```

Я собирал и проверял проект в WSL Ubuntu.

У меня использовались:

```
g++
cmake
Linux / WSL окружение
```

---

## 4. Сборка

Из корня проекта:

```
cmake -S . -B build
cmake --build build
```

После сборки исполняемый файл будет здесь:

```
build/minilang
```

Можно также так:

```
rm -rf build
mkdir build
cd build
cmake ..
make -j$(nproc)
cd ..
```

---

## 5. Запуск программы

Пример запуска:

```
./build/minilang examples/hello.ml
```

По умолчанию компилятор:

```
читает файл
запускает lexer
строит AST
проверяет семантику
генерирует bytecode
запускает VM
```

То есть программа сразу выполняется.

---

## 6. Флаги командной строки

Есть такие флаги:

```
--dump-tokens
--dump-ast
--dump-bytecode
--emit-asm
--no-run
--time-phases
-o <file>
--help
```

Что они делают:

```
--dump-tokens     вывести токены
--dump-ast        вывести AST
--dump-bytecode   вывести байткод
--emit-asm        сгенерировать NASM asm
--no-run          скомпилировать, но не запускать VM
--time-phases     вывести время фаз компиляции
-o <file>         задать файл вывода для asm
--help            вывести справку
```

Примеры:

```
./build/minilang examples/hello.ml --dump-tokens

./build/minilang examples/hello.ml --dump-ast

./build/minilang examples/hello.ml --dump-bytecode

./build/minilang examples/hello.ml --time-phases

./build/minilang examples/hello.ml --emit-asm -o build/hello.asm
```

Если asm-файл не задавать через `-o`, то у меня может создаваться `out.asm`.

---

## 7. Минимальная программа

Самая маленькая нормальная программа:

```
int main() {
    return 0;
}
```

`main` должен возвращать `int`.

Код возврата `main` становится кодом завершения программы.

---

## 8. Пример с переменными

```
int main() {
    int x = 10;
    uint u = cast<uint>(20);
    float64 y = 2.5;
    bool b = true;
    string s = "hello";

    print(s, "\n");

    return 0;
}
```

`uint` лучше создавать через `cast<uint>`, потому что обычный целый литерал имеет тип `int`.

---

## 9. var

`var` позволяет не писать тип явно.

```
int main() {
    var x = 10;
    var y = 2.5;
    var s = "hello";
    var b = true;

    print(typeid(x), "\n");
    print(typeid(y), "\n");
    print(typeid(s), "\n");
    print(typeid(b), "\n");

    return 0;
}
```

Компилятор сам выводит тип переменной по правой части.

После этого переменная всё равно имеет статический тип.

---

## 10. let

`let` создаёт неизменяемую переменную.

```
int main() {
    let x = 10;

    print(x, "\n");

    return 0;
}
```

Так нельзя:

```
let x = 10;
x = 20;
```

Будет semantic error.

---

## 11. Массивы

Массивы имеют фиксированный размер.

```
int main() {
    int[3] a = [1, 2, 3];

    print("len = ", len(a), "\n");
    print("a[0] = ", a[0], "\n");

    a[1] = 10;

    print("a[1] = ", a[1], "\n");

    return 0;
}
```

Если выйти за границы массива, будет runtime error.

Например:

```
int[2] a = [10, 20];
print(a[2]);
```

---

## 12. Массивы разных типов

Поддерживаются массивы не только `int`.

Примеры:

```
uint[3] flags = [cast<uint>(1), cast<uint>(2), cast<uint>(4)];

float64[2] limits = [0.0, 100.0];

bool[2] checks = [true, false];

string[2] names = ["Almas", "Bob"];
```

Можно делать массивы структур.

---

## 13. Структуры

Пример структуры:

```
struct Point {
    int x;
    int y;
}

int main() {
    Point p = Point { x: 10, y: 20 };

    print("x = ", p.x, "\n");
    print("y = ", p.y, "\n");

    p.x = 30;

    print("new x = ", p.x, "\n");

    return 0;
}
```

Структуры имеют nominal typing.

То есть две структуры с одинаковыми полями, но разными именами — разные типы.

---

## 14. Массивы внутри структур

Можно хранить массивы внутри структур:

```
struct Group {
    string title;
    int[3] grades;
    string[2] names;
}

int main() {
    Group g = Group {
        title: "VM group",
        grades: [90, 80, 70],
        names: ["Almas", "Bob"]
    };

    print(g.grades[0], "\n");

    g.grades[1] = 100;

    print(g.grades[1], "\n");

    return 0;
}
```

Это показывает, что вместе работают структуры, массивы, поля и индексирование.

---

## 15. Type aliases

Можно давать типам другие имена:

```
type Meters = int;

int main() {
    Meters distance = 100;
    int x = distance;

    return x;
}
```

`Meters` не создаёт новый отдельный тип.

Это просто другое имя для `int`.

---

## 16. Generic-функции

Есть generic-функции.

```
T id<T>(T x) {
    return x;
}

int main() {
    int a = id<int>(42);
    string s = id("hello");

    print(a, "\n");
    print(s, "\n");

    return 0;
}
```

Первый вызов:

```
id<int>(42)
```

использует явный type argument.

Второй вызов:

```
id("hello")
```

использует автовывод типа.

Компилятор сам понимает, что `T = string`.

Реализация generic ближе к type erasure: типы проверяются на этапе semantic analysis, но отдельная копия функции под каждый тип не создаётся.

---

## 17. Перегрузка функций

Есть перегрузка функций.

Можно написать несколько функций с одним именем, но разными параметрами:

```
int describe(int x) {
    return 1;
}

int describe(string x) {
    return 2;
}

int main() {
    return describe(10) + describe("hello");
}
```

Компилятор выбирает нужную функцию по типу аргументов.

Если две функции имеют одинаковые параметры, это ошибка.

---

## 18. Перегрузка с неявным приведением

Если точной перегрузки нет, компилятор может попробовать безопасное числовое приведение.

Поддерживаются:

```
int  -> float64
uint -> float64
```

Пример:

```
int onlyFloat(float64 x) {
    return 10;
}

int main() {
    return onlyFloat(42);
}
```

Тут `42` имеет тип `int`, а функция принимает `float64`.

Компилятор сам вставляет приведение к `float64`.

Но точное совпадение всегда важнее приведения.

---

## 19. Битовые операции

Есть битовые операции:

```
&
|
^
~
<<
>>
```

Пример:

```
int main() {
    int a = 6 & 3;
    int b = 6 | 3;
    int c = 6 ^ 3;
    int d = ~0;
    int e = 1 << 3;
    int f = 8 >> 1;

    print(a, "\n");
    print(b, "\n");
    print(c, "\n");
    print(d, "\n");
    print(e, "\n");
    print(f, "\n");

    return 0;
}
```

Для сдвигов есть runtime-проверка.

Нельзя сдвигать на отрицательное число или на 32 и больше.

---

## 20. Метафункции

Есть:

```
sizeof
typeid
typeof
```

Пример:

```
int main() {
    int x = 42;
    string s = "hello";

    print(sizeof(x), "\n");
    print(typeid(x), "\n");
    print(typeof(s), "\n");

    return 0;
}
```

`sizeof` возвращает размер типа.

`typeid` и `typeof` возвращают строковое имя типа.

---

## 21. Встроенные функции

Есть встроенные функции:

```
print
input
len
assert
exit
panic
```

Примеры:

```
print("hello\n");

string s = input();

int n = len("hello");

assert(n == 5, "bad length");

panic("something went wrong");
```

---

## 22. ASM backend

В проекте есть дополнительная генерация NASM ассемблера.

Пример:

```
./build/minilang examples/hello.ml --emit-asm --no-run
```

После этого создаётся asm-файл, например:

```
out.asm
```

Запуск:

```
nasm -f elf64 out.asm -o out.o
gcc -no-pie out.o -o out
./out
```

ASM backend экспериментальный.

Он поддерживает не весь язык.

Основной backend — это bytecode VM.

---

## 23. Тесты

Обычная проверка проекта:

```
cmake --build build
./tests/run_tests.sh
```

Если есть asm-тесты:

```
scripts/test_asm.sh
```

Если есть golden tests:

```
make golden
ctest --output-on-failure
```

Ожидаемо должно быть без ошибок.

---

## 24. Golden tests

Golden tests — это тесты, где сравниваются:

```
stdout
stderr
exit code
```

То есть тест проверяет не только то, что программа запустилась, но и то, что она вывела правильный текст и завершилась с правильным кодом.

У меня для этого есть скрипт:

```
scripts/run_golden_tests.py
```

Пример запуска:

```
./scripts/run_golden_tests.py --minilang ./build/minilang
```

Или через CMake:

```
make golden
```

---

## 25. Диагностика ошибок

Ошибки компиляции выводятся примерно так:

```
file:line:column: error: message
```

Например:

```
examples/test.ml:3:10: error: cannot initialize int variable with string value
```

Runtime-ошибки выводятся так:

```
runtime error: message at line N
```

Примеры runtime-ошибок:

```
division by zero
modulo by zero
array index out of bounds
invalid shift count
assert failure
panic
```

---

## 26. Спецификация

Спецификация лежит в папке:

```
specs/
```

Там описаны:

```
grammar
types
semantics
codegen
generics
function overloading
golden tests
standard library, если она добавлена
```

Основные файлы:

```
specs/grammar.md
specs/types.md
specs/semantics.md
specs/codegen.md
```

---

## 27. Ограничения

Не реализовано:

```
отдельный char
generic-структуры
указатели
nullable-типы
лямбды
функции как значения
перегрузка операторов
полноценная модульная система
IR-оптимизации
REPL
```

Перегрузка функций уже реализована, поэтому её здесь не пишем как ограничение.

ASM backend есть, но он не покрывает весь язык.

Полный основной backend — это байткод и виртуальная машина.



Коротко:

```
MiniLangVM — это компилятор C/Java-подобного языка. Он строит AST, проверяет семантику и типы, генерирует собственный байткод и выполняет его на стековой виртуальной машине.
```

Ещё:

```
Основной backend — VM. ASM backend дополнительный и покрывает подмножество языка.
```

Про допы:

```
В проекте есть generic-функции с автовыводом типа, перегрузка функций, перегрузка с неявными приведениями, meta-функции, bitwise-операции, golden tests и другие расширения.
```

Про ошибки:

```
Ошибки типов ловятся на этапе semantic analysis. Runtime-ошибки ловятся уже VM: деление на ноль, выход массива за границы, неправильный сдвиг, assert и panic.
```
