int32 show(string s) {
    print("s = ", s);
    return 7;
}

int main() {
    int32 x = show("hello from asm");
    print("x = ", x);

    return x;
}
