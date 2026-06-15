int main() {
    int32 x = 42;
    uint u = 42;
    float64 y = 3.14;
    bool b = true;
    string s = "hello";
    int32[3] a = [1, 2, 3];

    assert(sizeof(x) == 4, "sizeof int32 failed");
    assert(sizeof(u) == 4, "sizeof uint failed");
    assert(sizeof(y) == 8, "sizeof float64 failed");
    assert(sizeof(b) == 1, "sizeof bool failed");
    assert(sizeof(s) == 8, "sizeof string failed");
    assert(sizeof(a) == 12, "sizeof int32[3] failed");

    assert(typeid(x) == "int", "typeid int failed");
    assert(typeid(u) == "uint", "typeid uint failed");
    assert(typeid(y) == "float64", "typeid float64 failed");
    assert(typeid(b) == "bool", "typeid bool failed");
    assert(typeid(s) == "string", "typeid string failed");
    assert(typeof(s) == "string", "typeof string failed");

    return 0;
}
