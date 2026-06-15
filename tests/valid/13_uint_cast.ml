int main() {
    int32 x = 42;
    uint u = cast<uint>(x);
    int32 y = cast<int32>(u);

    assert(y == 42, "uint cast failed");

    return 0;
}
