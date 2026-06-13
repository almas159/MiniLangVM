int32 add(int32 a, int32 b) {
    return a + b;
}

int main() {
    int32 x = 10;
    int32 y = 20;
    int32 z = add(x, y);

    print("z = ", z);

    return z;
}
