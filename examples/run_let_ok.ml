int main() {
    let int32 x = 10;
    int32 y = 20;

    y = y + x;

    print("y = ", y, "\n");

    assert(y == 30, "let ok failed");

    return 0;
}
