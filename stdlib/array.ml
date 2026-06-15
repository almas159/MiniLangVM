int array_sum_int3(int[3] a) {
    return a[0] + a[1] + a[2];
}

int array_max_int3(int[3] a) {
    int m = a[0];

    if (a[1] > m) {
        m = a[1];
    }

    if (a[2] > m) {
        m = a[2];
    }

    return m;
}

bool array_contains_int3(int[3] a, int x) {
    return a[0] == x || a[1] == x || a[2] == x;
}

bool array_all_positive_int3(int[3] a) {
    return a[0] > 0 && a[1] > 0 && a[2] > 0;
}

string array_join3_strings(string[3] a) {
    return a[0] + a[1] + a[2];
}
