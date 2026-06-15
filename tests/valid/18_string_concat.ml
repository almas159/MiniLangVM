int main() {
    string a = "hello";
    string b = " world";
    string c = a + b;

    assert(c == "hello world", "string concat failed");

    return 0;
}
