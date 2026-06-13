uint make() {
    return 7;
}

int main() {
    uint x = make();
    print("x=", x, "\n");
    return cast<int>(x);
}
