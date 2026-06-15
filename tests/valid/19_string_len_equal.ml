int main() {
    string a = "hello";
    string b = "hello";
    string c = "world";

    assert(len(a) == 5, "string len failed");
    assert(a == b, "string equality failed");
    assert(a != c, "string inequality failed");

    return 0;
}
