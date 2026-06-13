int main() {
    int x = 10;
    int y = 20;
    bool ok = x < y;
    string s = "hello";

    if (ok) {
        print(s);
    } else {
        print("bad");
    }

    while (x < 15) {
        x = x + 1;
    }

    return x;
}
