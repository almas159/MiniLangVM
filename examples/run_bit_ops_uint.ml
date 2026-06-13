int main() {
    uint a = 6;
    uint b = 3;

    uint x = a & b;
    uint y = a | b;
    uint z = a ^ b;
    uint s = a << 1;

    print("x = ", x, "\n");
    print("y = ", y, "\n");
    print("z = ", z, "\n");
    print("s = ", s, "\n");

    assert(x == 2, "uint bit and failed");
    assert(y == 7, "uint bit or failed");
    assert(z == 5, "uint bit xor failed");
    assert(s == 12, "uint shift left failed");

    return 0;
}
