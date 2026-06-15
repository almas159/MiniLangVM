int32 fact(int32 n) {
    if (n <= 1) {
        return 1;
    }

    return n * fact(n - 1);
}

int main() {
    print("fact = ", fact(5));
    return 0;
}
