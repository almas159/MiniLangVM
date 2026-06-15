int main() {
    float64 a = inf;
    float64 b = -inf;
    float64 c = NaN;

    assert(a > 1000000.0, "asm inf failed");
    assert(b < -1000000.0, "asm -inf failed");

    print("special floats ok");

    return 0;
}
