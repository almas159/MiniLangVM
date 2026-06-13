int main() {
    var x = 10;
    var y = 2.5;
    var s = "hello";
    var b = true;

    print("x = ", x, "\n");
    print("y = ", y, "\n");
    print("s = ", s, "\n");
    print("b = ", b, "\n");

    assert(typeid(x) == "int", "var int inference failed");
    assert(typeid(y) == "float64", "var float inference failed");
    assert(typeid(s) == "string", "var string inference failed");
    assert(typeid(b) == "bool", "var bool inference failed");

    return 0;
}
