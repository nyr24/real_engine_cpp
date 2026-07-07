#include "basic.hpp"
#include "string.hpp"
#include "darray.hpp"
#include "allocators.hpp"
#include "slice.hpp"
#include "hashmap.hpp"
#include "io.hpp"
#include "conversions.hpp"
#include "math.hpp"
#include <cstdlib>

using namespace rg;

s32 rand_in_range(s32 start, s32 end)
{
    return (rand() % (end - start)) + start;
}

void populate_buff_ascii(char* buff, sz capacity)
{
    char* curr = buff;
    char* end = buff + capacity;
    for (; curr != end; ++curr)
    {
        *curr = rand_in_range('a', 'z');
    }
}

void print_buff_ascii(char* buff, sz capacity)
{
    printfn("%.*s", (s32)capacity, buff);
}

s32 main() {
    HeapAlloc mem;
    heap_init(&mem);
    Arena* arena = arena_create((Allocator*)&mem);
    defer (arena->destroy());

    HashmapSOA<FString<12>, s32> map;
    map.init(arena, 128);

    // HashmapSOA<FString<12>, s32> map;
    // map.init(arena);

    const sz COUNT = 256;
    const sz KEY_LEN = 12;
    FString<KEY_LEN> keys[COUNT];
    s32 values[COUNT];

    for (sz i = 0; i < COUNT; ++i)
    {
        populate_buff_ascii(keys[i].data, KEY_LEN);
        keys[i].count = KEY_LEN;
        values[i] = rand_in_range(0, 1024);
        map.put(keys[i], values[i]);
    }

    for (sz i = 0; i < COUNT; ++i)
    {
        ASSERT(*map.get(keys[i]) == values[i]);
    }

    for (sz i = 0; i < COUNT; ++i)
    {
        map.remove(keys[i]);
    }

    for (sz i = 0; i < COUNT; ++i)
    {
        ASSERT(map.get(keys[i]) == null);
    }

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
    
    return 0;
}
