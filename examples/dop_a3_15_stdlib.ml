int main() {
    print("=== stdlib demo ===\n");

    int[3] nums = [1, 2, 3];
    string[3] parts = ["A", "B", "C"];

    int a = math_abs(-10);
    int b = math_clamp(15, 0, 10);
    int c = math_square(4);
    int sum = array_sum_int3(nums);
    int maxValue = array_max_int3(nums);

    bool hasTwo = array_contains_int3(nums, 2);
    bool allPositive = array_all_positive_int3(nums);
    bool even = math_is_even(10);

    string repeated = string_repeat3("ha");
    string joined = array_join3_strings(parts);
    string surrounded = string_surround("[", "ok", "]");

    int cloned = memory_clone(9);
    string selected = std_select(false, "bad", "good");

    io_print_label_int("abs = ", a);
    io_print_label_int("clamp = ", b);
    io_print_label_int("square = ", c);
    io_print_label_int("sum = ", sum);
    io_print_label_string("joined = ", joined);

    if (a == 10 &&
        b == 10 &&
        c == 16 &&
        sum == 6 &&
        maxValue == 3 &&
        hasTwo &&
        allPositive &&
        even &&
        repeated == "hahaha" &&
        joined == "ABC" &&
        surrounded == "[ok]" &&
        cloned == 9 &&
        selected == "good") {
        print("STDLIB OK\n");
        return 0;
    }

    print("STDLIB FAILED\n");
    return 1;
}
