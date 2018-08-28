#include <stdio.h>
#include <stdlib.h>

#include "minunit.h"
#include "region.h"

int tests_run = 0;

/* Utility functions */

#define assign_region(x, u, d, l, r, s) do { \
    x.up    = u; \
    x.down  = d; \
    x.left  = l; \
    x.right = r; \
    x.scale = s; \
} while (0)

bool region_cmp(struct region *real, struct region *exp) {
    return real->up    == exp->up
        && real->down  == exp->down
        && real->left  == exp->left
        && real->right == exp->right
        && real->scale == exp->scale;
}

/* Tests for utility functions */

test region_cmp_equality() {
    struct region a, b;

    assign_region(a, 1, 2, 3, 4, 5);
    assign_region(b, 1, 2, 3, 4, 5);

    mu_assert("region_cmp inaccurate", region_cmp(&a, &b));
    return 0;
}

/* intersection */

test intersection_a_in_b() {
    struct region a, b, real, exp, *ret_ptr;

    assign_region(a, 3,  7, 3,  7, 1);
    assign_region(b, 0, 10, 0, 10, 1);

    ret_ptr = intersection(&a, &b);
    mu_assert("a-in-b intersection returns NULL", ret_ptr != NULL);

    real = *ret_ptr;
    exp = a;
    mu_assert("a-in-b intersection inaccurate", region_cmp(&real, &exp));

    free(ret_ptr);
    return 0;
}

test intersection_b_in_a() {
    struct region a, b, real, exp, *ret_ptr;

    assign_region(a,  0, 100,  0, 100, 1);
    assign_region(b, 25,  30, 25,  30, 1);

    ret_ptr = intersection(&a, &b);
    mu_assert("b-in-a intersection returns NULL", ret_ptr != NULL);

    real = *ret_ptr;
    exp = b;
    mu_assert("b-in-a intersection inaccurate", region_cmp(&real, &exp));

    free(ret_ptr);
    return 0;
}

test intersection_partial_overlap() {
    struct region a, b, real, exp, *ret_ptr;

    assign_region(a,  0, 20,  0, 20, 1);
    assign_region(b, 10, 30, 10, 30, 1);

    ret_ptr = intersection(&a, &b);
    mu_assert("partial-overlap intersection returns NULL", ret_ptr != NULL);

    real = *ret_ptr;
    assign_region(exp, 10, 20, 10, 20, 1);
    mu_assert("partial-overlap intersection inaccurate",
            region_cmp(&real, &exp));

    free(ret_ptr);
    return 0;
}

test intersection_no_overlap() {
    struct region a, b, *ret_ptr;

    assign_region(a, 0, 10,  0, 10, 1);
    assign_region(b, 0, 10, 10, 20, 1);

    ret_ptr = intersection(&a, &b);
    mu_assert("no overlap still receiving intersection", ret_ptr == NULL);

    return 0;
}

/* overlaps */

test overlaps_partial() {
    struct region a, b;

    assign_region(a, 0, 10, 0, 10, 1);
    assign_region(b, 5, 15, 5, 15, 1);

    mu_assert("no overlap found", overlaps(&a, &b));

    return 0;
}

test overlaps_not() {
    struct region a, b;

    assign_region(a,  0, 10,  0, 10, 1);
    assign_region(b, 20, 30, 20, 30, 1);

    mu_assert("overlap found", !overlaps(&a, &b));

    return 0;
}

test overlaps_edge() {
    struct region a, b;

    assign_region(a,  0, 10,  0, 10, 1);
    assign_region(b, 10, 20, 10, 20, 1);

    mu_assert("adjacent tiles count as overlapping", !overlaps(&a, &b));

    return 0;
}

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
    mu_run_test(region_cmp_equality);

    mu_run_test(intersection_a_in_b);
    mu_run_test(intersection_b_in_a);
    mu_run_test(intersection_partial_overlap);
    mu_run_test(intersection_no_overlap);

    mu_run_test(overlaps_partial);
    mu_run_test(overlaps_not);
    mu_run_test(overlaps_edge);

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
