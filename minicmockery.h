#ifndef _MINICMOCKERY_H_
#define _MINICMOCKERY_H_

typedef void (*UnitTestFunction)(void **state);

typedef enum UnitTestFunctionType
{
    UNIT_TEST_FUNCTION_TYPE_TEST = 0,
    UNIT_TEST_FUNCTION_TYPE_SETUP,
    UNIT_TEST_FUNCTION_TYPE_TEARDOWN,
} UnitTestFunctionType;

typedef struct UnitTest
{
    const char *name;
    UnitTestFunction function;
    UnitTestFunctionType function_type;
} UnitTest;

#define assert_int_equal(a, b) \
    _assert_int_equal((const int)a, (const int)b, __FILE__, __LINE__)
#define assert_int_not_equal(a, b) \
    _assert_int_not_equal((const int)a, (const int)b, __FILE__, __LINE__)

#define unit_test(f)                        \
    {                                       \
#f, f, UNIT_TEST_FUNCTION_TYPE_TEST \
    }
#define run_tests(tests) _run_tests(tests, sizeof(tests) / sizeof(tests)[0])

void _assert_int_equal(const int a, const int b, const char *const file, const int line);
void _assert_int_not_equal(const int a, const int b, const char *const file, const int line);
int _run_tests(const UnitTest *const tests, const size_t number_of_tests);

#endif