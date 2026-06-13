int main() {
    int32[3] a = [1, 2, 3];
    int32[3] b = [1, 2, 3];
    int32[3] c = [1, 2, 4];

    assert(a == b, "array equality failed");
    assert(a != c, "array inequality failed");

    print("array equality ok\n");

    return 0;
}
