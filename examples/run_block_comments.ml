int main() {
    /*
        This is a block comment.
        It must be ignored by lexer.
    */

    int32 x = 10;

    /* comment inside code */
    int32 y = 20;

    print("sum = ", x + y, "\n");

    assert(x + y == 30, "block comment failed");

    return 0;
}
