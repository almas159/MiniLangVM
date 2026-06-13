int main() {
    print("before panic\n");

    panic("something went wrong");

    print("after panic\n");

    return 0;
}
