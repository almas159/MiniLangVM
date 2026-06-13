int main() {
    uint a = 10;
    uint b = 20;

    assert(a < b, "uint < failed");
    assert(b > a, "uint > failed");
    assert(a != b, "uint != failed");
    assert(a == 10, "uint == int literal failed");

    print("asm uint compare ok");

    return 0;
}
