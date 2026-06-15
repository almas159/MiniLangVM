T choose<T>(bool cond, T a, T b) {
    if (cond) {
        return a;
    }

    return b;
}

int main() {
    int32 x = choose(true, 10, "bad");

    return 0;
}
