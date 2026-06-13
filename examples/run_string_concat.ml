int main() {
    string a = "hello";
    string b = " world";
    string c = a + b;

    print(c, "\n");

    assert(c == "hello world", "string concat failed");

    return 0;
}
