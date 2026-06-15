T std_identity<T>(T x) {
    return x;
}

T std_select<T>(bool cond, T a, T b) {
    if (cond) {
        return a;
    }

    return b;
}
