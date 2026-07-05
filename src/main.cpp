#include "basic.hpp"
#include "string.hpp"
#include "darray.hpp"
#include "allocators.hpp"
#include "slice.hpp"
#include "hashmap.hpp"
#include "io.hpp"

using namespace rg;

s32 main() {
    HeapAlloc mem;
    heap_init(&mem);
    Arena* arena = arena_create((Allocator*)&mem);
    defer (arena->destroy());

    // s32 arr[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    // Slice<s32> init_values{ CARRAY_SIZED(arr) };

    // DArray<s32> darr;
    // darr.init_slice(arena, init_values);
    // defer(darr.destroy());
    // darr.foreach_ref([](s32* val) { *val = *val * 2; });

    // for (const s32& v : darr)
    // {
    //     printfn("%d", v);
    // }

    // StrView sv{ CSTR_SIZED("Hello, 世界 🌍") };

    // DString* ds = dstring_create_cstr(arena, s);
    // StrView view = ds->view();
    // prs32fn("string: %.*s, length: %lu", s32(view.count), view.ptr, view.count);

    // ds->foreach_codepos32([](Codepos32& pos32) { prs32fn("codepos32: %04x", pos32); });
 
    // FString<24> str = sv;
    // str.foreach_codepoint([](Utf8Codepoint point) { printfn("codepos32: %04x", point); });

    // Hashmap<StrView, s32> map;
    // map.init(arena);

    // StrView key1 = { CSTR_SIZED("hello") };
    // StrView key2 = { CSTR_SIZED("world") };
    // StrView key3 = { CSTR_SIZED("third") };

    // map.put(key1, 123);
    // map.put(key2, 288);
    // map.put(key3, 399);

    // ASSERT(*map.get(key1) == 123);
    // ASSERT(*map.get(key2) == 288);
    // ASSERT(*map.get(key3) == 399);

    // map.foreach_value([](s32 val) { printfn("Map has value: %d", val); });
    // map.foreach_key([](StrView key) { printfn("Map has key: %s", key.ptr); });

    Path path;
    FString<100> init_path(CSTR_SIZED("../build/debug"));
    path.init(arena, init_path.slice());
    printfn("Path is: " FMT_STR_LEN, FMT_DSTRING_VAL(path));

    return 0;
}
