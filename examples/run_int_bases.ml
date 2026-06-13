int main() {
    int32 x = 0x2A;
    int32 y = 0b101010;
    int32 z = 42;

    print("x = ", x, "\n");
    print("y = ", y, "\n");
    print("z = ", z, "\n");

    assert(x == 42, "hex literal failed");
    assert(y == 42, "binary literal failed");
    assert(z == 42, "decimal literal failed");

    return 0;
}
