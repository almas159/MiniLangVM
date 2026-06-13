// Проверка лексера

int main() {
    int x = 10;
    int y = 20;

    if (x < y && y != 0) {
        print("x = ", x);
    } else {
        print("bad");
    }

    /*
       многострочный комментарий
    */

    while (x < 15) {
        x = x + 1;
    }

    return x;
}
