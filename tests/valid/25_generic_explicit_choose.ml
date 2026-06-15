T choose<T>(bool cond, T a, T b) {
    if (cond) {
        return a;
    }

    return b;
}

int main() {
    int32 x = choose<int32>(true, 10, 20);
    string s = choose<string>(false, "left", "right");

    assert(x == 10, "explicit generic choose int failed");
    assert(s == "right", "explicit generic choose string failed");

    return 0;
}
