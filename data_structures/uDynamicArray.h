#ifndef __uDynamicArray__
#define __uDynamicArray__ 1

#include <engine_tools/type_tools.h>

// Optimize for random access, "push back", "pop last"
typedef struct
{
    /* void (*push)(void* data); */
    /* void (*pop)(); */
    /* void (*at)(size_t index); */
    void (*const push)(void*);
    /* void (*const pop)(); */
    void (*const at)(size_t);

    const size_t num_elements;
    const size_t scaling_factor;
    /* const size_t scale_count; */
    /* const size_t num_first_level_indices; */
    const size_t datatype_size;
    const size_t max_elements;
    // const size_t estimated_maximum_elements;

    /* const void** const data; */
    void* data;
} uDynamicArray;

void uDAPush(uDynamicArray* const da, void* const data);

void* uDAIndex(uDynamicArray* const da, const size_t index);

#define uDAInit(type) uAPI_DAInit(sizeof(type))
extern uDynamicArray* uAPI_DAInit(const size_t datatype_size_in);

/* extern void uDestroyDynamicArray(uDynamicArray* da); */
#endif // __uDynamicArray
