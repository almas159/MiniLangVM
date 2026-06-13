struct Data {
    string[2] names;
}

int main() {
    Data d = Data {
        names: ["aa", "bb"]
    };

    d.names[0] = 123;

    return 0;
}
