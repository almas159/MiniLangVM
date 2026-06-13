struct Point {
    int32 x;
    int32 y;
}

int main() {
    int32[3] a = [1, 2, 3];
    uint[3] u = [1, 2, 3];
    float64[2] f = [1.5, 2.5];
    bool[2] b = [true, false];
    string[2] s = ["aa", "bb"];

    Point[2] p = [
        Point { x: 1, y: 2 },
        Point { x: 3, y: 4 }
    ];

    print("a[1]=", a[1], "\n");
    print("u[2]=", u[2], "\n");
    print("f[0]=", f[0], "\n");
    print("b[0]=", b[0], "\n");
    print("s[1]=", s[1], "\n");
    print("p[0]=", p[0], "\n");

    u[0] = 10;
    f[1] = 7.5;
    b[1] = true;
    s[0] = "hello";
    p[1] = Point { x: 9, y: 9 };

    print("u[0]=", u[0], "\n");
    print("f[1]=", f[1], "\n");
    print("b[1]=", b[1], "\n");
    print("s[0]=", s[0], "\n");
    print("p[1]=", p[1], "\n");

    if (len(s) == 2 && u[0] == cast<uint>(10) && s[0] == "hello") {
        return 0;
    }

    return 1;
}
