struct Point {
    int32 x;
    int32 y;
}

int main() {
    Point p = Point { x: 10, y: 20 };

    p.x = 100;

    print("x = ", p.x);
    print("y = ", p.y);

    return 0;
}
