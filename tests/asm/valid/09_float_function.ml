float64 half(float64 x) {
    return x / 2.0;
}

float64 addf(float64 a, float64 b) {
    return a + b;
}

int main() {
    float64 x = half(8.0);
    float64 y = addf(x, 1.5);

    print("x = ", x);
    print("y = ", y);

    return 0;
}
