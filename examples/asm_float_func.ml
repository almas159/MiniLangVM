float64 half(float64 x) {
    return x / 2.0;
}

float64 addf(float64 a, float64 b) {
    return a + b;
}

int main() {
    float64 x = half(7.0);
    float64 y = addf(x, 1.25);

    print("x = ", x);
    print("y = ", y);

    int32 result = cast<int32>(y);

    print("result = ", result);

    return result;
}
