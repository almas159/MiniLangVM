int main() {
    int32 sum = 0;

    for (int32 i = 0; i < 10; i++) {
        if (i == 2) {
            continue;
        }

        if (i == 6) {
            break;
        }

        sum += i;
    }

    print(sum);

    return sum;
}
