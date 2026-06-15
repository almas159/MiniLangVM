int onlyInt(int x) {
    print("only int: ", x, "\n");
    return 1;
}

int onlyUInt(uint x) {
    print("only uint: ", x, "\n");
    return 2;
}

int onlyFloat(float64 x) {
    print("only float: ", x, "\n");
    return 3;
}

int exact(int x) {
    print("exact int\n");
    return 10;
}

int exact(float64 x) {
    print("exact float\n");
    return 20;
}

int main() {
    int a = onlyFloat(10);             // int -> float64
    int b = onlyFloat(cast<uint>(7));  // uint -> float64

    int c = onlyUInt(5);               // int -> uint
    int d = onlyInt(cast<uint>(8));    // uint -> int

    int e = onlyInt(3.5);              // float64 -> int

    int f = exact(42);                 // exact int
    int g = exact(2.5);                // exact float64

    int result = a + b + c + d + e + f + g;

    print("result = ", result, "\n");

    return result;
}
