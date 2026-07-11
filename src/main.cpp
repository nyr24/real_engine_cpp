#include "core/basic.hpp"
#include "core/allocators.hpp"
#include "core/conversions.hpp"

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

struct Entity
{
    FString<20> name;
    u32 age;

    void init(StrView name_, u32 age_)
    {
        this->name.init_view(name_);
        this->age = age_;
    }
    void dance() { printfn("Entity named: " FMT_PLACEHOLDER_LEN " is dancing!", FMT_FSTRING_VAL(this->name)); }
};

s32 main()
{
    // HeapAlloc mem;
    // mem.init();
    VmemAllocator* vmem = VmemAllocator::create(1 * GB);
    defer(vmem->destroy());
    // Arena* arena = Arena::create(mem, 1 << 14);

// Pool allocator test.
    PoolAllocator* pool = PoolAllocator::create(vmem, sizeof(Entity), alignof(Entity), 128);
    defer(pool->destroy());

    char temp_buf[20];
    
    for (sz i = 0; i < 256; ++i)
    {
        populate_buff_ascii(temp_buf, 20);
        auto* en = (Entity*)pool->allocate();
        en->init({temp_buf, 20}, rand_in_range(10, 99));
        en->dance();
        pool->free(en);
    }

// Push test Test.
    // const sz COUNT = 100;
    // const sz CHARS_IN_STR = 30;

    // DArray<FString<CHARS_IN_STR>> write_arr;
    // write_arr.init(mem, COUNT);
    // DArray<FString<CHARS_IN_STR>> read_arr;
    // read_arr.init(mem, COUNT);

    // char temp_buff[CHARS_IN_STR];

    // for (sz i = 0; i < COUNT; ++i)
    // {
    //     populate_buff_ascii(temp_buff, CHARS_IN_STR);
    //     read_arr.push(StrView{temp_buff, CHARS_IN_STR});
    // }

    // s64 start = clock();

    // for (sz i = 0; i < COUNT; ++i)
    // {
    //     write_arr.push(read_arr[i]);
    // }

    // s64 end = clock();
    // printfn("spend time: %ld", end - start);
// Vmem test.
    // char* block_1024 = (char*)allocator_allocate(mem, 1024);
    // char* block_512 = (char*)allocator_allocate(mem, 512);
    // char* block_256 = (char*)allocator_allocate(mem, 256);
    // char* block_256_2 = (char*)allocator_allocate(mem, 256);
    // allocator_display_info(mem);
    // allocator_free(mem, block_512);
    // allocator_free(mem, block_256_2);
    // allocator_display_info(mem);
    // allocator_free(mem, block_256);
    // allocator_free(mem, block_1024);
    // allocator_display_info(mem);

// TLSF.
    // void* memory = allocator_allocate(mem, 16384);

    // tlsf_t Tlsf = tlsf_create_with_pool(memory, 16384);
    // defer(tlsf_destroy(Tlsf));

    // char* block_1024 = (char*)tlsf_malloc(Tlsf, 1024);
    // char* block_512 = (char*)tlsf_malloc(Tlsf, 512);
    // char* block_256 = (char*)tlsf_malloc(Tlsf, 256);
    // char* block_256_2 = (char*)tlsf_malloc(Tlsf, 256);
    // populate_buff_ascii(block_1024, 24);
    // populate_buff_ascii(block_512, 24);
    // printfn("first block: %.*s", 24, block_1024);
    // printfn("second block: %.*s", 24, block_512);
    // void* tlsf_realloc(tlsf_t tlsf, void* ptr, size_t size);
    // void tlsf_free(tlsf_t tlsf, void* ptr);

// Map benchmark.

    // const sz COUNT = 100'000'00;
    // const sz KEY_LEN = 12;
    // const sz MAP_INIT_CAP = 128;

    // DArray<FString<KEY_LEN>> keys;
    // keys.init(arena, COUNT);

    // DArray<s32> values;
    // values.init(arena, COUNT);

    // Hashmap<FString<12>, s32> map;
    // map.init(arena, MAP_INIT_CAP);

    // for (sz i = 0; i < COUNT; ++i)
    // {
    //     FString<12> new_key;
    //     populate_buff_ascii(new_key.data, KEY_LEN);
    //     new_key.count = KEY_LEN;
    //     s32 new_val = rand_in_range(0, 1024);
    //     keys.push(new_key);
    //     values.push(new_val);
    // }

    // sz start = clock();

    // for (sz i = 0; i < COUNT; ++i)
    // {
    //     map.put(keys[i], values[i]);
    // }

    // for (sz i = 0; i < COUNT; ++i)
    // {
    //     ASSERT(*map.get(keys[i]) == values[i]);
    // }

    // for (sz i = 0; i < COUNT; ++i)
    // {
    //     map.remove(keys[i]);
    // }

    // for (sz i = 0; i < COUNT; ++i)
    // {
    //     ASSERT(map.get(keys[i]) == null);
    // }

    // sz end = clock();
    // LOG_INFO("Benchmark ended, result: %d ns\n", end - start);

// PATHS

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
