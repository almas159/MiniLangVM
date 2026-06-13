float64 half(float64 x) {
    return x / 2.0;
}

int main() {
    float64 a = 3.5;
    float64 b = 2.0;
    float64 c = a + b;

    print("c = ", c);
    print("half = ", half(c));
    print("cmp = ", c > 5.0);

    return 0;
}
