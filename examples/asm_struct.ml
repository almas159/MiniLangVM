struct Point {
    int32 x;
    int32 y;
}

int main() {
    Point p = Point { x: 10, y: 20 };

    print("x = ", p.x);
    print("y = ", p.y);

    p.x = 100;
    p.y = 200;

    print("new x = ", p.x);
    print("new y = ", p.y);

    return 0;
}
