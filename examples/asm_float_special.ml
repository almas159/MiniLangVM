int main() {
    float64 a = inf;
    float64 b = -inf;
    float64 c = NaN;

    print("a = ", a);
    print("b = ", b);
    print("c = ", c);

    assert(a > 1000000.0, "asm inf comparison failed");
    assert(b < -1000000.0, "asm -inf comparison failed");

    return 0;
}
