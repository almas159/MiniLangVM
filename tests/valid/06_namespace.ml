namespace Math {
    int32 square(int32 x) {
        return x * x;
    }

    int32 add(int32 a, int32 b) {
        return a + b;
    }
}

int main() {
    int32 x = Math::square(5);
    int32 y = Math::add(x, 10);

    print("x = ", x);
    print("y = ", y);

    return 0;
}
