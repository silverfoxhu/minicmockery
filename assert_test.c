#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "minicmockery.h"

int add(int a, int b)
{
    return a + b;
}

int sub(int a, int b)
{
    return a - b;
}

void test_add(void **state)
{
    assert_int_equal(add(3, 3), 6);
    assert_int_equal(add(3, -3), 0);
    assert_int_not_equal(add(1, 2), 1);
}

void test_sub(void **state)
{
    assert_int_equal(sub(3, 3), 0);
    assert_int_equal(sub(3, -3), 6);
    assert_int_not_equal(sub(2, 1), 0);
}

int main(int argc, char *argv[])
{
    const UnitTest tests[] = {
        unit_test(test_add),
        unit_test(test_sub),
    };
    return run_tests(tests);
}