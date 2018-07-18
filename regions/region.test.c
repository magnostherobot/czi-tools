#include <stdio.h>
#include <stdlib.h>

#include "minunit.h"
#include "region.h"

#define assign_region(x, u, d, l, r, s) do { \
    x.up    = u; \
    x.down  = d; \
    x.left  = l; \
    x.right = r; \
    x.scale = s; \
} while (0)

int tests_run = 0;

bool region_cmp(struct region *real, struct region *exp) {
    return real->up    == exp->up
        && real->down  == exp->down
        && real->left  == exp->left
        && real->right == exp->right
        && real->scale == exp->scale;
}

test region_cmp_equality() {
    struct region a, b;

    assign_region(a, 1, 2, 3, 4, 5);
    assign_region(b, 1, 2, 3, 4, 5);

    mu_assert("region_cmp inaccurate", region_cmp(&a, &b));
    return 0;
}

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

test run_tests() {
    mu_run_test(region_cmp_equality);

    mu_run_test(intersection_a_in_b);
    mu_run_test(intersection_b_in_a);
    mu_run_test(intersection_partial_overlap);
    mu_run_test(intersection_no_overlap);

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
