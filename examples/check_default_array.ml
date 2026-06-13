int main() {
    int32[100] arr;

    arr[0] = 10;
    arr[1] = 20;

    print("arr[0] = ", arr[0], "\n");
    print("arr[1] = ", arr[1], "\n");

    assert(arr[0] == 10, "arr[0] failed");
    assert(arr[1] == 20, "arr[1] failed");

    return 0;
}
