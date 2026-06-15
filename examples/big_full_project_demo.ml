type Score = int;

struct Point {
    int x;
    int y;
}

struct Student {
    string name;
    Score score;
    bool passed;
}

struct Group {
    string title;
    Student[3] students;
    int[3] grades;
    uint[3] flags;
    float64[2] limits;
    Point[2] points;
}

T identity<T>(T x) {
    return x;
}

T choose<T>(bool cond, T a, T b) {
    if (cond) {
        return a;
    }

    return b;
}

int describe(int x) {
    print("describe(int): ", x, "\n");
    return 1;
}

int describe(string s) {
    print("describe(string): ", s, "\n");
    return 2;
}

int describe(float64 x) {
    print("describe(float64): ", x, "\n");
    return 3;
}

int onlyFloat(float64 x) {
    print("onlyFloat(float64): ", x, "\n");
    return 10;
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }

    return n * factorial(n - 1);
}

int sumGrades(int[3] a) {
    int i = 0;
    int sum = 0;

    while (i < len(a)) {
        sum = sum + a[i];
        i = i + 1;
    }

    return sum;
}

int countPassed(Student[3] students) {
    int i = 0;
    int count = 0;

    while (i < len(students)) {
        Student current = students[i];

        if (current.passed) {
            count = count + 1;
        }

        i = i + 1;
    }

    return count;
}

uint buildMask(uint[3] flags) {
    uint result = cast<uint>(0);
    int i = 0;

    while (i < len(flags)) {
        result = result | flags[i];
        i = i + 1;
    }

    return result;
}

int main() {
    print("=== MiniLangVM big demo ===\n");

    let immutableTitle = "Compiler project";
    var inferredNumber = 42;
    var inferredText = "hello";
    var inferredBool = true;

    print("let title = ", immutableTitle, "\n");
    print("var number = ", inferredNumber, "\n");
    print("var text = ", inferredText, "\n");
    print("var bool = ", inferredBool, "\n");

    Student a = Student {
        name: "Almas",
        score: 95,
        passed: true
    };

    Student b = Student {
        name: "Bob",
        score: 70,
        passed: true
    };

    Student c = Student {
        name: "Eve",
        score: 40,
        passed: false
    };

    Group g = Group {
        title: "VM group",
        students: [a, b, c],
        grades: [95, 70, 40],
        flags: [cast<uint>(1), cast<uint>(2), cast<uint>(4)],
        limits: [0.0, 100.0],
        points: [
            Point { x: 1, y: 2 },
            Point { x: 3, y: 4 }
        ]
    };

    print("\n--- structs and arrays ---\n");

    Student firstStudent = g.students[0];
    Student secondStudent = g.students[1];
    Point firstPoint = g.points[0];

    print("group title = ", g.title, "\n");
    print("first student = ", firstStudent.name, "\n");
    print("second score = ", secondStudent.score, "\n");
    print("points[0].x = ", firstPoint.x, "\n");

    g.students[2] = Student {
        name: "Charlie",
        score: 80,
        passed: true
    };

    g.grades[2] = 80;
    g.points[0] = Point { x: 10, y: 20 };

    Student thirdStudent = g.students[2];
    Point updatedPoint = g.points[0];

    print("updated third student = ", thirdStudent.name, "\n");
    print("updated third score = ", g.grades[2], "\n");
    print("updated points[0].x = ", updatedPoint.x, "\n");

    int total = sumGrades(g.grades);
    int passed = countPassed(g.students);

    print("sum grades = ", total, "\n");
    print("passed count = ", passed, "\n");

    print("\n--- generics ---\n");

    int genericInt = identity<int>(123);
    string genericString = identity("generic string");
    bool genericBool = choose(false, false, true);
    Student chosenStudent = choose(true, g.students[0], g.students[1]);

    print("identity<int>(123) = ", genericInt, "\n");
    print("identity(\"generic string\") = ", genericString, "\n");
    print("choose bool = ", genericBool, "\n");
    print("choose Student = ", chosenStudent.name, "\n");

    print("\n--- overloads ---\n");

    int d1 = describe(10);
    int d2 = describe("text");
    int d3 = describe(3.5);
    int d4 = onlyFloat(10);

    print("overload sum = ", d1 + d2 + d3 + d4, "\n");

    print("\n--- bitwise ---\n");

    uint mask = buildMask(g.flags);
    uint shifted = cast<uint>(1) << 3;
    uint mixed = (mask | shifted) ^ cast<uint>(2);
    uint inverted = ~cast<uint>(0);

    print("mask = ", mask, "\n");
    print("shifted = ", shifted, "\n");
    print("mixed = ", mixed, "\n");
    print("inverted = ", inverted, "\n");

    print("\n--- loops, break, continue ---\n");

    int loopSum = 0;

    for (int i = 0; i < 10; i = i + 1) {
        if (i == 3) {
            continue;
        }

        if (i == 8) {
            break;
        }

        loopSum = loopSum + i;
    }

    print("loopSum = ", loopSum, "\n");

    print("\n--- recursion ---\n");

    int fact5 = factorial(5);
    print("factorial(5) = ", fact5, "\n");

    print("\n--- float special values ---\n");

    float64 posInf = inf;
    float64 negInf = -inf;
    float64 nanValue = NaN;

    print("posInf = ", posInf, "\n");
    print("negInf = ", negInf, "\n");
    print("nanValue = ", nanValue, "\n");

    print("\n--- meta-functions ---\n");

    print("sizeof(inferredNumber) = ", sizeof(inferredNumber), "\n");
    print("typeid(inferredText) = ", typeid(inferredText), "\n");
    print("typeof(g.grades) = ", typeof(g.grades), "\n");

    print("\n--- final checks ---\n");

    if (total == 245 &&
        passed == 3 &&
        genericInt == 123 &&
        genericString == "generic string" &&
        genericBool &&
        chosenStudent.name == "Almas" &&
        d1 + d2 + d3 + d4 == 16 &&
        mask == cast<uint>(7) &&
        shifted == cast<uint>(8) &&
        mixed == cast<uint>(13) &&
        loopSum == 25 &&
        fact5 == 120) {
        print("ALL CHECKS PASSED\n");
        return 0;
    }

    print("CHECK FAILED\n");
    return 1;
}
