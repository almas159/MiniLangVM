T id<T>(T x) {
    return x;
}

int main() {
    int a = id(42);
    string s = id("hello");

    print(a, "\n");
    print(s, "\n");

    return 0;
}
