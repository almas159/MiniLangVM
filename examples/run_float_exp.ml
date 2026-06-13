int main() {
    float64 x = 1.5e2;
    float64 y = 2.5E1;

    print("x = ", x, "\n");
    print("y = ", y, "\n");

    assert(cast<int32>(x) == 150, "scientific literal x failed");
    assert(cast<int32>(y) == 25, "scientific literal y failed");

    return 0;
}
