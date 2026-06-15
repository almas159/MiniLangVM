int exact(int x) {
    print("exact int\n");
    return 1;
}

int exact(float64 x) {
    print("exact float\n");
    return 2;
}

int onlyFloat(float64 x) {
    print("only float\n");
    return 10;
}

int main() {
    int a = exact(42);       // выбирается int, точное совпадение
    int b = exact(3.5);      // выбирается float64, точное совпадение
    int c = onlyFloat(42);   // int неявно приводится к float64

    return a + b + c;
}
