int main() {
    uint a = 40;
    uint b = 2;
    uint c = a + b;

    print("c = ", c);

    assert(c == 42, "asm uint addition failed");

    return 0;
}
