int main() {
    int x = 0;

    while (x < 10) {
        x = x + 1;

        if (x == 3) {
            continue;
        }

        if (x == 7) {
            break;
        }

        print(x);
    }

    return 0;
}
