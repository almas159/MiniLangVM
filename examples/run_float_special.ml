int main() {
    float64 a = inf;
    float64 b = -inf;
    float64 c = NaN;

    print("a = ", a, "\n");
    print("b = ", b, "\n");
    print("c = ", c, "\n");

    assert(a > 1000000.0, "inf comparison failed");
    assert(b < -1000000.0, "-inf comparison failed");

    return 0;
}
