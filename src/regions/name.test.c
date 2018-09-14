#include <stdio.h>

#include "minunit.h"
#include "name.h"

int tests_run = 0;

/* set_side */

test set_side_simple() {
    struct dirent ent = { .d_name = "Ap2s5r7,Bp3s1r7" };
    char *id = "B";
    struct options opts = { .filename_value_base = 10 };
    czi_coord_t left, right;
    int scale;

    int ret = set_side(&ent, id, &opts, &left, &right, &scale);

    mu_assert("error value returned", !ret);
    mu_assert("left value incorrectly parsed", left == 3);
    mu_assert("right value incorrectly parsed", right == 4);
    mu_assert("scale incorrectly parsed", scale == 7);

    return 0;
}

test run_tests() {
    mu_run_test(set_side_simple);

    return 0;
}

int main(int argc, char *argv[]) {
    char *result = run_tests();
    if (result) {
        printf("%s\n", result);
    } else {
        printf("all tests passed.\n");
    }
    printf("%d tests run\n", tests_run);

    return result != 0;
}
