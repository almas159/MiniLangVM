int main() {
    int32 a = 6 & 3;
    int32 b = 6 | 3;
    int32 c = 6 ^ 3;
    int32 d = ~0;
    int32 e = 1 << 3;
    int32 f = 8 >> 1;

    print("a = ", a, "\n");
    print("b = ", b, "\n");
    print("c = ", c, "\n");
    print("d = ", d, "\n");
    print("e = ", e, "\n");
    print("f = ", f, "\n");

    assert(a == 2, "bit and failed");
    assert(b == 7, "bit or failed");
    assert(c == 5, "bit xor failed");
    assert(d == -1, "bit not failed");
    assert(e == 8, "shift left failed");
    assert(f == 4, "shift right failed");

    return 0;
}
