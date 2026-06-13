int main() {
    int a = 6;      // 110
    int b = 3;      // 011

    int x = a & b;  // 010 = 2
    int y = a | b;  // 111 = 7
    int z = a ^ b;  // 101 = 5
    int l = 1 << 4; // 16
    int r = 16 >> 2; // 4
    int n = ~0;     // -1

    print("x=", x, "\n");
    print("y=", y, "\n");
    print("z=", z, "\n");
    print("l=", l, "\n");
    print("r=", r, "\n");
    print("n=", n, "\n");

    if (x == 2 && y == 7 && z == 5 && l == 16 && r == 4 && n == -1) {
        return 0;
    }

    return 1;
}
