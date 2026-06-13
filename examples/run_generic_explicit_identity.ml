T id<T>(T x) {
    return x;
}

int main() {
    int32 a = id<int32>(42);
    string s = id<string>("hello");
    bool b = id<bool>(true);

    print("a = ", a, "\n");
    print("s = ", s, "\n");
    print("b = ", b, "\n");

    assert(a == 42, "explicit generic int failed");
    assert(s == "hello", "explicit generic string failed");
    assert(b, "explicit generic bool failed");

    return 0;
}
