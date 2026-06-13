int main() {
    uint a = cast<uint>(6);
    uint b = cast<uint>(3);

    uint x = a & b;
    uint y = a | b;
    uint z = a ^ b;
    uint l = cast<uint>(1) << 4;
    uint r = cast<uint>(16) >> 2;
    uint n = ~cast<uint>(0);

    print("x=", x, "\n");
    print("y=", y, "\n");
    print("z=", z, "\n");
    print("l=", l, "\n");
    print("r=", r, "\n");
    print("n=", n, "\n");

    if (x == cast<uint>(2) &&
        y == cast<uint>(7) &&
        z == cast<uint>(5) &&
        l == cast<uint>(16) &&
        r == cast<uint>(4)) {
        return 0;
    }

    return 1;
}
