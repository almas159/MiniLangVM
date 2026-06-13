int main() {
    float64 x = 3.5;
    float64 y = 2.0;
    float64 z = x + y * 2.0;

    print("z = ", z);
    print("z > 7.0 = ", z > 7.0);

    int32 a = cast<int32>(z);
    print("a = ", a);

    float64 b = cast<float64>(a);
    print("b = ", b);

    return a;
}
