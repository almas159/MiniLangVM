int main() {
    uint a = 40;
    uint b = 2;
    uint c = a + b;

    print("a = ", a, "\n");
    print("b = ", b, "\n");
    print("c = ", c, "\n");

    assert(c == 42, "uint addition failed");

    return 0;
}
