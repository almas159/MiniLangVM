int main() {
    int32 i = 0;
    int32 sum = 0;

    while (i < 5) {
        ;
        sum = sum + i;
        ;
        i++;
    }

    print("sum = ", sum, "\n");

    assert(sum == 10, "empty statement loop failed");

    return 0;
}
