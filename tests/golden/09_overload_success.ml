int describe(int x) {
    print("int: ", x, "\n");
    return 1;
}

int describe(string x) {
    print("string: ", x, "\n");
    return 2;
}

int describe(bool x) {
    print("bool: ", x, "\n");
    return 3;
}

int main() {
    int a = describe(10);
    int b = describe("hello");
    int c = describe(true);

    return a + b + c;
}
