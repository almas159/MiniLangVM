int string_length(string s) {
    return len(s);
}

bool string_empty(string s) {
    return len(s) == 0;
}

bool string_not_empty(string s) {
    return len(s) != 0;
}

string string_repeat2(string s) {
    return s + s;
}

string string_repeat3(string s) {
    return s + s + s;
}

string string_surround(string left, string s, string right) {
    return left + s + right;
}
