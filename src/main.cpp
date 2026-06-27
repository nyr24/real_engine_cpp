#include "basic.hpp"
#include "string.hpp"
#include "dynarr.hpp"
#include "allocators.hpp"

using namespace rg;

int main() {
    HeapAlloc mem;
    heap_init(&mem);
    Arena* arena = arena_create((Allocator*)&mem);
    defer (arena->destroy());

    int arr[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    View<int> init_values = view_create(arr, 10);

    DArray<s32>* dynarr = darray_create_with_values(arena, init_values);
    defer(dynarr->destroy());
    dynarr->foreach([](s32& val) { val = val * 2; });

    for (const s32& v : *dynarr)
    {
        printfn("%d", v);
    }

    const char* s = "Hello, 世界 🌍";
    // for (s32 i = 0; i < slen; ++i)
    // {
    //     printfn("char: %08x", s[i]);
    // }

    DString* ds = dstring_create_cstr(arena, s);
    StrView view = ds->view();
    printfn("string: %.*s, length: %lu", s32(view.count), view.ptr, view.count);
    
    return 0;
}
