int main() {
    int32 x = 42;
    float64 y = 3.14;
    bool b = true;
    string s = "hello";

    print("sizeof int = ", sizeof(x), "\n");
    print("sizeof float = ", sizeof(y), "\n");
    print("sizeof bool = ", sizeof(b), "\n");
    print("sizeof string = ", sizeof(s), "\n");

    print("typeid bool = ", typeid(b), "\n");
    print("typeid string = ", typeid(s), "\n");
    print("typeof string = ", typeof(s), "\n");

    assert(sizeof(x) == 4, "sizeof int failed");
    assert(sizeof(y) == 8, "sizeof float failed");
    assert(sizeof(b) == 1, "sizeof bool failed");
    assert(sizeof(s) == 8, "sizeof string reference failed");

    assert(typeid(b) == "bool", "typeid bool failed");
    assert(typeid(s) == "string", "typeid string failed");
    assert(typeof(s) == "string", "typeof string failed");

    return 0;
}
