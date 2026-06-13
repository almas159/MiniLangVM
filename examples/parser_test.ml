int main() {
    int x = 2 + 3 * 4;
    int y = 10;

    if (x < y) {
        print("x меньше y");
    } else {
        print("x больше или равен y");
    }

    while (x < 20) {
        x = x + 1;
    }

    print("x = ", x);

    return x;
}
