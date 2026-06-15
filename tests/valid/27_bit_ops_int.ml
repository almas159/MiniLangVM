int main() {
    int32 a = 6 & 3;
    int32 b = 6 | 3;
    int32 c = 6 ^ 3;
    int32 d = ~0;
    int32 e = 1 << 3;
    int32 f = 8 >> 1;

    assert(a == 2, "int bit and failed");
    assert(b == 7, "int bit or failed");
    assert(c == 5, "int bit xor failed");
    assert(d == -1, "int bit not failed");
    assert(e == 8, "int shift left failed");
    assert(f == 4, "int shift right failed");

    return 0;
}
