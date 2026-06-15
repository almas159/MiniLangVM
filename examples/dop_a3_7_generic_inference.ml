T identity<T>(T x) {
    return x;
}

T choose<T>(bool cond, T a, T b) {
    if (cond) {
        return a;
    } else {
        return b;
    }
}

int main() {
    int a = identity(42);
    string s = identity("hello");
    bool b = identity(true);

    int x = choose(true, 10, 20);
    string y = choose(false, "left", "right");

    print("a = ", a, "\n");
    print("s = ", s, "\n");
    print("b = ", b, "\n");
    print("x = ", x, "\n");
    print("y = ", y, "\n");

    return 0;
}
