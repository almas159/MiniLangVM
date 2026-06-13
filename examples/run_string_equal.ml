int main() {
    string a = "hello";
    string b = "hello";
    string c = "world";

    assert(a == b, "string equality failed");
    assert(a != c, "string inequality failed");

    print("string equality ok\n");

    return 0;
}
