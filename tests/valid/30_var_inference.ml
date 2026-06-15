int main() {
    var x = 10;
    var y = 2.5;
    var s = "hello";
    var b = true;

    assert(typeid(x) == "int", "var int inference failed");
    assert(typeid(y) == "float64", "var float inference failed");
    assert(typeid(s) == "string", "var string inference failed");
    assert(typeid(b) == "bool", "var bool inference failed");

    return 0;
}
