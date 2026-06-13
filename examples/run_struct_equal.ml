struct Point {
    int32 x;
    int32 y;
}

int main() {
    Point p = Point { x: 1, y: 2 };
    Point q = Point { x: 1, y: 2 };
    Point r = Point { x: 1, y: 3 };

    assert(p == q, "struct equality failed");
    assert(p != r, "struct inequality failed");

    print("struct equality ok\n");

    return 0;
}
