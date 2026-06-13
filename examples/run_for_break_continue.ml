int main() {
    for (int i = 0; i < 8; i = i + 1) {
        if (i == 2) {
            continue;
        }

        if (i == 6) {
            break;
        }

        print(i);
    }

    return 0;
}
