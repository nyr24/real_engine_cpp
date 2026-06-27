#include "basic.hpp"
#include "dynarr.hpp"
#include "allocators.hpp"

using namespace rg;

int main() {
    HeapAlloc mem;
    heap_init(&mem);

    int arr[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    View<int> init_values = view_create(arr, 10);

    Arena* arena = arena_create((Allocator*)&mem, 4096);

    printfn("arena addr: %p", arena);
    
    DynArray<s32>* dynarr = dynarr_create_with_values((Allocator*)arena, init_values);
    defer(dynarr->destroy());

    printn("hello: ");
    printfn("hello, %d", 10);

    s32 n1 = 10;
    s32 n2 = 255;

    printfn("max is: %d", max(n1, n2));
    printfn("min is: %d", min(n1, n2));
    printfn("clamp is: %d", clamp(n1, 50, 210));

    s64 n3 = s64(n1);

    for (int v : *dynarr)
    {
        printf("%d ", v);
    }
    printn("\n");

    return 0;
}
