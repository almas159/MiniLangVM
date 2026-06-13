void compressSequence(int32[] arr, int32 length) {
    if (length == 0) {
        print("empty\n");
    } else {
        int32 current = arr[0];
        int32 count = 1;
        int32 i = 1;

        while (i < length) {
            if (arr[i] == current) {
                count = count + 1;
            } else {
                print("(", current, ",", count, ") ");
                current = arr[i];
                count = 1;
            }

            i = i + 1;
        }

        print("(", current, ",", count, ")\n");
    }
}

void printNumberTriangle(int32 n) {
    int number = 1;
    int row = 1;

    while (row <= n) {
        int col = 1;

        while (col <= row) {
            print(number, " ");
            number = number + 1;
            col = col + 1;
        }

        print("\n");
        row = row + 1;
    }
}

int main() {
    int[10] arr = [1, 1, 1, 2, 3, 3, 4, 4, 4, 4];

    print("Task 3: compress sequence");
    compressSequence(arr, 10);

    print("Task 4: number triangle");
    printNumberTriangle(4);

    return 0;
}
