int math_abs(int x) {
    if (x < 0) {
        return -x;
    }

    return x;
}

int math_max(int a, int b) {
    if (a > b) {
        return a;
    }

    return b;
}

int math_min(int a, int b) {
    if (a < b) {
        return a;
    }

    return b;
}

int math_clamp(int x, int low, int high) {
    if (x < low) {
        return low;
    }

    if (x > high) {
        return high;
    }

    return x;
}

int math_square(int x) {
    return x * x;
}

bool math_is_even(int x) {
    return x % 2 == 0;
}

bool math_is_odd(int x) {
    return x % 2 != 0;
}
