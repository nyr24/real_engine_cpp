#include "basic.hpp"
#include "string.hpp"
#include "darray.hpp"
#include "allocators.hpp"
#include "slice.hpp"
#include "hashmap.hpp"
#include "io.hpp"
#include "conversions.hpp"
#include "math.hpp"

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

    // Path path;
    // FString<100> init_path(CSTR_SIZED("/home/nyr/dev/c++/real_engine/build/debug"));
    // path.init(arena, init_path.slice());
    // printfn("Path is: " FMT_STR_LEN, FMT_DSTRING_VAL(path));

    // Conversions test.
    
    // s32 int_val   = -1224;
    // u32 uint_val  = 32228;
    // f32 float_val = -22.464;

    // FString<20> int_val_s   = { CSTR_SIZED("-1224") };
    // FString<20> uint_val_s  = { CSTR_SIZED("32228") };
    // FString<20> float_val_s = { CSTR_SIZED("-22.464") };

    // s32 int_res = string_to_int<s32>(int_val_s.slice());
    // u32 uint_res = string_to_uint<u32>(uint_val_s.slice());
    // f32 float_res = string_to_float<f32>(float_val_s.slice());

    // printfn("int: %d, uint: %u, float: %f", int_res, uint_res, float_res);

    // FString<20> int_val_s2;
    // FString<20> uint_val_s2;
    // FString<20> float_val_s2;

    // Slice<char> int_res2 = int_to_string<s32>(int_res, int_val_s2.data, 20);
    // Slice<char> uint_res2 = uint_to_string<u32>(uint_res, uint_val_s2.data, 20);
    // Slice<char> float_res2 = float_to_string<f32>(float_res, float_val_s2.data, 20);

    // printfn("Reverse conversions:\nint_str: %.*s, uint_str: %.*s, float_str: %.*s,",
    //     FMT_SLICE(int_res2), FMT_SLICE(uint_res2), FMT_SLICE(float_res2));
    
    Mat4 a = Mat4::translation({ 1, 2, 3 }); 
    Mat4 b = Mat4::scale({ 2, 2, 2 }); 
    Mat4 res = a * b;
    res *= res;

    // f32 arr[16];
    // _mm512_storeu_ps(arr, res.repr);
    // res.to_arr(arr);

    for (f32 val : res.arr)
    {
        printfn("%f ", val);
    }
    
    return 0;
}
