void compressSequence(int32[100] arr, int32 length) {
    if (length <= 0) {
        print("empty\n");
        return;
    }

    int32 current = arr[0];
    int32 count = 1;
    int32 i = 1;

    while (i < length) {
        if (arr[i] == current) {
            count++;
        } else {
            print("(", current, ",", count, ") ");

            current = arr[i];
            count = 1;
        }

        i++;
    }

    print("(", current, ",", count, ")\n");
}

void printNumberTriangle(int32 n) {
    int32 value = 1;
    int32 row = 1;

    while (row <= n) {
        int32 col = 1;

        while (col <= row) {
            print(value);

            if (col < row) {
                print(" ");
            }

            value++;
            col++;
        }

        print("\n");
        row++;
    }
}

int main() {
    int32[100] arr;

    print("Введите длину последовательности: ");
    int32 length = toInt(input());

    if (length < 0) {
        print("не должна быть отрицательной\n");
        return 1;
    }

    if (length > 100) {
        print("длина должна быть <= 100\n");
        return 1;
    }

    int32 i = 0;

    while (i < length) {
        print("arr[", i, "] = ");
        arr[i] = toInt(input());
        i++;
    }

    compressSequence(arr, length);

    print("\n");

    print("Введите n для треугольника: ");
    int32 n = toInt(input());

    if (n < 0) {
        print("n не должно быть отрицательной\n");
        return 1;
    }

    printNumberTriangle(n);
    return 0;
}