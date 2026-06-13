int main() {
    let a = 10;
    var b = 5;

    uint x = 2;
    uint y = cast<uint>(3);

    uint z = x + y;
    uint m = z * cast<uint>(4);
    uint d = m / cast<uint>(2);
    uint r = d % cast<uint>(5);

    print("a=", a, "\n");
    print("z=", z, "\n");
    print("m=", m, "\n");
    print("d=", d, "\n");
    print("r=", r, "\n");

    if (z < m && m >= d && r == cast<uint>(0)) {
        return cast<int>(z);
    }

    return 1;
}
