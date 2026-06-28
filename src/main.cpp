#include "basic.hpp"
#include "string.hpp"
#include "darray.hpp"
#include "allocators.hpp"

using namespace rg;

int main() {
    HeapAlloc mem;
    heap_init(&mem);
    Arena* arena = arena_create((Allocator*)&mem);
    defer (arena->destroy());

    int arr[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    View<int> init_values = view_create(arr, 10);

    DArray<s32>* darr = darray_create_with_values(arena, init_values);
    defer(darr->destroy());
    darr->foreach([](s32& val) { val = val * 2; });

    for (const s32& v : *darr)
    {
        printfn("%d", v);
    }

    const char* s = "Hello, 世界 🌍";

    // DString* ds = dstring_create_cstr(arena, s);
    // StrView view = ds->view();
    // printfn("string: %.*s, length: %lu", s32(view.count), view.ptr, view.count);

    // ds->foreach_codepoint([](Codepoint& point) { printfn("codepoint: %04x", point); });
 
    FString<24> str;
    str.init_cstr(s);
    str.foreach_codepoint([](Codepoint& point) { printfn("codepoint: %04x", point); });
    
    return 0;
}
