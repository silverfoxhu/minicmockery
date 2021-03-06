#include <stdio.h>
#include <setjmp.h>
#include "minicmockery.h"
#include <bits/types.h>
#include <malloc.h>
#include <string.h>

// Size of guard bytes around dynamically allocated blocks.
#define MALLOC_GUARD_SIZE 16
// Pattern used to initialize guard blocks.
#define MALLOC_GUARD_PATTERN 0xEF
// Alignment of allocated blocks.  NOTE: This must be base2.
#define MALLOC_ALIGNMENT sizeof(size_t)
// Pattern used to initialize memory allocated with test_malloc().
#define MALLOC_ALLOC_PATTERN 0xBA
#define MALLOC_FREE_PATTERN 0xCD

// Calculates the number of elements in an array.
#define ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

#define container_of(ptr, type, member) ({\
const typeof( ((type *)0)->member ) *__mptr = (ptr); \
(type *)( (char *)__mptr - offsetof(type,member) ); })

typedef struct ListNode
{
    const void *value;
    struct ListNode *next;
    struct ListNode *prev;
} ListNode;

// Location within some source code.
typedef struct SourceLocation
{
    const char *file;
    int line;
} SourceLocation;

// Debug information for malloc().
typedef struct MallocBlockInfo
{
    void *block;             // Address of the block returned by malloc().
    size_t allocated_size;   // Total size of the allocated block.
    size_t size;             // Request block size.
    SourceLocation location; // Where the block was allocated.
    ListNode node;           // Node within list of all allocated blocks.
} MallocBlockInfo;

typedef struct SymbolValue
{
    SourceLocation location;
    const void *value;
    ListNode node;
} SymbolValue;

typedef struct
{
    jmp_buf run_test_env;
    __uint32_t failed_count;

    // List of all currently allocated blocks.
    ListNode allocated_blocks;
    // List of all function results
    ListNode function_results;
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

// Initialize a list node.
static ListNode *list_initialize(ListNode *const node)
{
    node->next = node;
    node->prev = node;
    return node;
}

// Add new_node to the end of the list.
static ListNode *list_add(ListNode *const head, ListNode *new_node)
{
    new_node->next = head;
    new_node->prev = head->prev;
    head->prev->next = new_node;
    head->prev = new_node;
    return new_node;
}

// Remove a node from a list.
static ListNode *list_remove(ListNode *const node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;

    return node;
}

/* Remove a list node from a list and free the node. */
/*
static void list_remove_free(ListNode *const node)
{
    free(list_remove(node));
}
*/

static ListNode *get_allocated_blocks_list()
{
    if (NULL == minicmockery_entity.allocated_blocks.value)
    {
        list_initialize(&minicmockery_entity.allocated_blocks);
        minicmockery_entity.allocated_blocks.value = (void *)1;
    }

    return &minicmockery_entity.allocated_blocks;
}

static ListNode *get_function_results_list()
{
    if (NULL == minicmockery_entity.function_results.value)
    {
        list_initialize(&minicmockery_entity.function_results);
        minicmockery_entity.function_results.value = (void *)1;
    }
    return &minicmockery_entity.function_results;
}

// Set a source location.
static void set_source_location(SourceLocation *const location, const char *const file, const int line)
{
    location->file = file;
    location->line = line;
}

void *_test_malloc(const size_t size, const char *file, const int line)
{
    MallocBlockInfo *block_info;
    ListNode *const block_list = get_allocated_blocks_list();
    const size_t allocate_size = size + (MALLOC_GUARD_SIZE * 2) + sizeof(*block_info) + MALLOC_ALIGNMENT;

    char *const block = (char *)malloc(allocate_size);

    // Calculate the returned address.
    char *ptr = (char *)(((size_t)block + MALLOC_GUARD_SIZE + sizeof(*block_info) +
                          MALLOC_ALIGNMENT) &
                         ~(MALLOC_ALIGNMENT - 1));

    // Initialize the guard blocks.
    memset(ptr - MALLOC_GUARD_SIZE, MALLOC_GUARD_PATTERN, MALLOC_GUARD_SIZE);
    memset(ptr + size, MALLOC_GUARD_PATTERN, MALLOC_GUARD_SIZE);
    memset(ptr, MALLOC_ALLOC_PATTERN, size);

    block_info = (MallocBlockInfo *)(ptr - (MALLOC_GUARD_SIZE + sizeof(*block_info)));
    set_source_location(&block_info->location, file, line);
    block_info->allocated_size = allocate_size;
    block_info->size = size;
    block_info->block = block;
    block_info->node.value = block_info;
    list_add(block_list, &block_info->node);
    return ptr;
}

void _test_free(void *const ptr, const char *file, const int line)
{
    unsigned int i;
    char *block = (char *)ptr;
    MallocBlockInfo *block_info;
    block_info = (MallocBlockInfo *)(block - (MALLOC_GUARD_SIZE + sizeof(*block_info)));
    char *guards[2] = {block - MALLOC_GUARD_SIZE, block + block_info->size};
    int corrupt_happened = 0;
    for (i = 0; i < ARRAY_LENGTH(guards); i++)
    {
        unsigned int j;
        unsigned char *const guard = guards[i];
        for (j = 0; j < MALLOC_GUARD_SIZE; j++)
        {
            if (MALLOC_GUARD_PATTERN != guard[j])
            {
                printf("Guard block of 0x%08lx size=%ld allocated by %s:%d at 0x%08lx is corrupt\n",
                       (size_t)ptr, block_info->size,
                       block_info->location.file, block_info->location.line,
                       (size_t)&guard[j]);
                corrupt_happened = 1;
            }
        }
    }
    list_remove(&block_info->node);
    block = block_info->block;
    memset(block, MALLOC_FREE_PATTERN, block_info->allocated_size);
    free(block);
    if (1 == corrupt_happened)
    {
        _fail(file, line);
    }
}

static void _fail_if_allocated_block_not_empty(const char *const test_name)
{
    ListNode *const block_list = get_allocated_blocks_list();
    ListNode *node;
    ListNode *node_tmp;
    __uint32_t allocated_block_num = 0;

    for (node = block_list->prev, node_tmp = node->prev;
         node != block_list;
         node = node_tmp, node_tmp = node->prev)
    {
        const MallocBlockInfo *const block_info = node->value;
        allocated_block_num++;
        list_remove(node);
        printf("ERROR: leaked block allocated in %s: %d\n", block_info->location.file, block_info->location.line);
        free(block_info->block);
    }
    if (0 != allocated_block_num)
    {
        printf("ERROR: %s leaked %d block(s)\n", test_name, allocated_block_num);
        longjmp(*(_run_test_env_get()), 1);
    }
}

void _will_return(const char *const function_name, const char *const file, const int line, const void *const value)
{
    ListNode *function_results_list = get_function_results_list();
    SymbolValue *const return_value = malloc(sizeof(*return_value));
    return_value->value = value;
    set_source_location(&return_value->location, file, line);
    list_add(function_results_list, &(return_value->node));
}

void *_mock(const char *const function, const char *const file, const int line)
{
    ListNode *function_results_list = get_function_results_list();
    ListNode *node;
    for (node = function_results_list->prev; function_results_list != node->prev; node = node->prev)
    { //do nothing};
    }

    SymbolValue *return_value = container_of(node, SymbolValue, node);
    void *value = (void *)return_value->value;
    list_remove(node);
    free(return_value);
    return value;
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
            _fail_if_allocated_block_not_empty(test->name);
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
