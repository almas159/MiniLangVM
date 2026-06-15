T choose<T>(bool cond, T a, T b) {
    if (cond) {
        return a;
    }

    return b;
}

int main() {
    int32 x = choose<string>(true, 10, 20);

    return 0;
}
