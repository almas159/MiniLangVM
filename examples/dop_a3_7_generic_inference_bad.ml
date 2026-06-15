T choose<T>(bool cond, T a, T b) {
    if (cond) {
        return a;
    } else {
        return b;
    }
}

int main() {
    int x = choose(true, 10, "wrong");
    return x;
}
