#include <stdio.h>
#include <setjmp.h>
#include "minicmockery.h"
#include <bits/types.h>

typedef struct
{
    jmp_buf run_test_env;
    __uint32_t failed_count;
} minicmockery_entity_t;
static minicmockery_entity_t minicmockery_entity;

static jmp_buf *_run_test_env_get()
{
    return &(minicmockery_entity.run_test_env);
}

static __uint32_t _failed_count_get()
{
    return minicmockery_entity.failed_count;
}

static void _failed_count_add()
{
    ++minicmockery_entity.failed_count;
}

static void _fail(const char *const file, const int line)
{
    printf("%s: %d\n", file, line);
    longjmp(*(_run_test_env_get()), 1);
}

void _assert_int_equal(const int a, const int b, const char *const file,
                       const int line)
{
    if (a != b)
    {
        printf("%d != %d\n", a, b);
        _fail(file, line);
    }
}

void _assert_int_not_equal(const int a, const int b, const char *const file,
                           const int line)
{
    if (a == b)
    {
        printf("%d != %d\n", a, b);
        _fail(file, line);
    }
}
int _run_tests(const UnitTest *const tests, const size_t number_of_tests)
{
    __uint32_t index = 0;
    const UnitTest *test = NULL;
    __int8_t ret = 0;
    void *state = NULL;
    for (index = 0; index < number_of_tests; index++)
    {
        test = &tests[index];
        printf("%s: begin test ---\n", test->name);
        ret = setjmp(*_run_test_env_get());
        if (0 == ret)
        {
            test->function(&state);
            printf("%s: test completely successed\n", test->name);
        }
        else
        {
            printf("%s: test failed\n", test->name);
            _failed_count_add();
        }
    }
    if (0 != _failed_count_get())
    {
        printf("%u of %ld test cases failed\n", _failed_count_get(), number_of_tests);
    }
    else
    {
        printf("All %lu test cases successed\n", number_of_tests);
    }

    return 0;
}
