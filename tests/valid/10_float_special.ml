int main() {
    float64 a = inf;
    float64 b = -inf;
    float64 c = NaN;

    assert(a > 1000000.0, "inf failed");
    assert(b < -1000000.0, "-inf failed");

    print("special floats ok\n");

    return 0;
}
