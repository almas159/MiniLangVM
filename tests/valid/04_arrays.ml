int main() {
    int32[3] a = [10, 20, 30];

    a[1] = 99;

    print(a[0]);
    print(a[1]);
    print(a[2]);
    print(a);
    print("len = ", len(a));

    return 0;
}
