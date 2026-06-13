struct Point {
    int32 x;
    int32 y;
}

int main() {
    Point p = Point { x: 10, y: 20 };

    print(p.x);
    print(p.y);
    print(p);

    return p.x;
}
