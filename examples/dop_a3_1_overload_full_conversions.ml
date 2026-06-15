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
    int a = onlyFloat(10);
    int b = onlyFloat(cast<uint>(7));
    int c = onlyUInt(5);
    int d = onlyInt(cast<uint>(8));
    int e = onlyInt(3.5);

    int f = exact(42);
    int g = exact(2.5);

    return a + b + c + d + e + f + g;
}
