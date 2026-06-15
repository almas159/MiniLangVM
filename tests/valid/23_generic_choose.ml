T choose<T>(bool cond, T a, T b) {
    if (cond) {
        return a;
    }

    return b;
}

int main() {
    int32 x = choose(true, 10, 20);
    string s = choose(false, "left", "right");

    assert(x == 10, "generic choose int failed");
    assert(s == "right", "generic choose string failed");

    return 0;
}
