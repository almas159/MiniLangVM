int main() {
    int32 a = 10;
    int32 b = 20;

    assert(a < b, "comparison < failed");
    assert(b > a, "comparison > failed");

    print("comparison ok\n");

    return 0;
}
