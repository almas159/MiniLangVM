T identity<T>(T x) {
    return x;
}

int main() {
    int a = identity<int>(42);
    string s = identity<string>("hello");
    bool b = identity<bool>(true);

    print("a = ", a, "\n");
    print("s = ", s, "\n");
    print("b = ", b, "\n");

    return 0;
}
