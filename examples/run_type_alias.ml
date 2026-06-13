type Meters = int32;

Meters add(Meters a, Meters b) {
    return a + b;
}

int main() {
    Meters x = 10;
    Meters y = 15;
    Meters z = add(x, y);

    print("z = ", z);

    return z;
}
