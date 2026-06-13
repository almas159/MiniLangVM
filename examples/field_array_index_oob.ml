struct Data {
    string[2] names;
}

int main() {
    Data d = Data {
        names: ["aa", "bb"]
    };

    print(d.names[2]);

    return 0;
}
