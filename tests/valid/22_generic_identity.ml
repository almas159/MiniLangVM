T id<T>(T x) {
    return x;
}

int main() {
    int32 a = id(42);
    string s = id("hello");
    bool b = id(true);

    assert(a == 42, "generic int failed");
    assert(s == "hello", "generic string failed");
    assert(b, "generic bool failed");

    return 0;
}
