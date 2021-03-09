#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <malloc.h>
#include "minicmockery.h"

extern void *_test_malloc(const size_t size, const char *file, const int line);
extern void *_test_calloc(const size_t number_of_elements, const size_t size,
                          const char *file, const int line);
extern void _test_free(void *const ptr, const char *file, const int line);

#define malloc(size) _test_malloc(size, __FILE__, __LINE__)
#define calloc(num, size) _test_calloc(num, size, __FILE__, __LINE__)
#define free(ptr) _test_free(ptr, __FILE__, __LINE__)

int add(int a, int b)
{
    return a + b;
}

int sub(int a, int b)
{
    return a - b;
}

void leak_memory()
{
    int *const temporary = (int *)malloc(sizeof(int));
    *temporary = 0;
}

void buffer_overflow()
{
    char *const memory = (char *)malloc(sizeof(int));
    memory[sizeof(int)] = '!';
    free(memory);
}

void buffer_underflow()
{
    char *const memory = (char *)malloc(sizeof(int));
    memory[-1] = '!';
    free(memory);
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

void leak_memory_test(void **state)
{
    leak_memory();
}

// Test case that fails as buffer_overflow() corrupts an allocated block.
void buffer_overflow_test(void **state)
{
    buffer_overflow();
}

// Test case that fails as buffer_underflow() corrupts an allocated block.
void buffer_underflow_test(void **state)
{
    buffer_underflow();
}

int main(int argc, char *argv[])
{
    const UnitTest tests[] = {
        unit_test(test_add),
        unit_test(test_sub),
        unit_test(leak_memory_test),
        unit_test(buffer_overflow_test),
        unit_test(buffer_underflow_test),
    };
    return run_tests(tests);
}