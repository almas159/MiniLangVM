int add(int a, int b) {
    return a + b;
}

int square(int x) {
    return x * x;
}

bool isPositive(int x) {
    return x > 0;
}

string hello() {
    return "hello";
}

int main() {
    int x = add(2, 3);
    int y = square(x);

    print("x = ", x);
    print("y = ", y);
    print("positive = ", isPositive(y));
    print(hello());

    return y;
}
