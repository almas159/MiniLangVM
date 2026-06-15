T id<T>(T x) {
    return x;
}

int main() {
    int32 x = id<int32, string>(42);

    return 0;
}
