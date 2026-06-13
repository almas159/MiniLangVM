int main() {
    int32 a = 10;
    int32 b = 20;

    assert(a < b, "less failed");
    assert(b > a, "greater failed");

    print("comparison after generics ok\n");

    return 0;
}
