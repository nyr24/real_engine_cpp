#include "core/basic.hpp"
#include "core/allocators.hpp"
#include "core/conversions.hpp"
#include "collections/thread_pool.hpp"
#include "core/io.hpp"
#include "core/testing.hpp"
#include "engine/entry.hpp"

using namespace rg;

s32 thread_read_file(void* arg)
{
    Path* file_path = (Path*)arg;
    Arena* temp_alloc = get_temp_allocator();

    TEMP_ALLOC_SCOPE(temp_alloc);

    auto [res, is_ok] = file_read(temp_alloc, file_path);

    if (!is_ok)
    {
        LOG_ERROR("Failed to read data from file: ", FMT_PLACEHOLDER_LEN, FMT_DSTRING_VAL(res));
        reset_log_scope();
        return 1;
    }

    LOG_SCOPED("First bytes!: " FMT_PLACEHOLDER_LEN "\n", FMT_SLICE(res.slice(0, 256)));

    const sz PRIME_CALC = 24; 
    for (sz i = 0; i < PRIME_CALC; ++i)
    {
        s32 random_val = rand_in_range(1'776'454, 8'323'323);
        s32 next_p = next_prime(random_val);
        LOG_SCOPED("next prime from %d is %d\n", random_val, next_p);
    }

    return 0;
}

s32 main()
{
    application_init();
    defer(application_destroy());

    set_log_scope(LogLevel::INFO);
    // defer(reset_log_scope());

    Arena* arena = get_temp_allocator();

    // Threadpool and Io.
    ThreadPool<4, 16> tpool;
    tpool.init();
    defer(tpool.destroy());

    Array<Path, 16> file_paths;
    file_paths[0].init(arena, StrView{ CSTR_SIZED("assets/temp/temp1.txt") });
    file_paths[1].init(arena, StrView{ CSTR_SIZED("assets/temp/temp2.txt") });
    file_paths[2].init(arena, StrView{ CSTR_SIZED("assets/temp/temp3.txt") });
    file_paths[3].init(arena, StrView{ CSTR_SIZED("assets/temp/temp4.txt") });
    file_paths[4].init(arena, StrView{ CSTR_SIZED("assets/temp/temp5.txt") });
    file_paths[5].init(arena, StrView{ CSTR_SIZED("assets/temp/temp6.txt") });
    file_paths[6].init(arena, StrView{ CSTR_SIZED("assets/temp/temp7.txt") });
    file_paths[7].init(arena, StrView{ CSTR_SIZED("assets/temp/temp8.txt") });
    file_paths[8].init(arena, StrView{ CSTR_SIZED("assets/temp/temp9.txt") });
    file_paths[9].init(arena, StrView{ CSTR_SIZED("assets/temp/temp10.txt") });
    file_paths[10].init(arena, StrView{ CSTR_SIZED("assets/temp/temp11.txt") });
    file_paths[11].init(arena, StrView{ CSTR_SIZED("assets/temp/temp12.txt") });
    file_paths[12].init(arena, StrView{ CSTR_SIZED("assets/temp/temp13.txt") });
    file_paths[13].init(arena, StrView{ CSTR_SIZED("assets/temp/temp14.txt") });
    file_paths[14].init(arena, StrView{ CSTR_SIZED("assets/temp/temp15.txt") });

    tpool.submit_task({ thread_read_file, &file_paths[0] }); 
    tpool.submit_task({ thread_read_file, &file_paths[1] }); 
    tpool.submit_task({ thread_read_file, &file_paths[2] }); 
    tpool.submit_task({ thread_read_file, &file_paths[3] }); 
    tpool.submit_task({ thread_read_file, &file_paths[4] }); 
    tpool.submit_task({ thread_read_file, &file_paths[5] }); 
    tpool.submit_task({ thread_read_file, &file_paths[6] }); 
    tpool.submit_task({ thread_read_file, &file_paths[7] }); 
    tpool.submit_task({ thread_read_file, &file_paths[8] }); 
    tpool.submit_task({ thread_read_file, &file_paths[9] }); 
    tpool.submit_task({ thread_read_file, &file_paths[10] }); 
    tpool.submit_task({ thread_read_file, &file_paths[11] }); 
    tpool.submit_task({ thread_read_file, &file_paths[12] }); 
    tpool.submit_task({ thread_read_file, &file_paths[13] }); 
    tpool.submit_task({ thread_read_file, &file_paths[14] }); 

    // ThreadTask tasks[3] = {
    //     { thread_read_file, &file_paths[0] },
    //     { thread_read_file, &file_paths[0] },
    //     { thread_read_file, &file_paths[0] },
    // };

    // tpool.submit_task_many({ tasks, 3 });

    tpool.await();

    LOG_DEBUG("MAIN THREAD: await is over");

// Atomic
    // Atomic<s32> atom = {200};
    // atom.add(228);
    // atom.sub(404);
    // atom.bit_and(0b00001111);
    // printfn("val: %d", atom.val);
    
// RingBuffer
    // RingBuffer<s32, 256> rb;
    // rb.push(1);
    // rb.push(2);
    // rb.push(3);

    // s32 out;
    // rb.pop(&out);
    // printfn("popped: %d", out);
    // rb.pop(&out);
    // printfn("popped: %d", out);
    // rb.pop(&out);
    // printfn("popped: %d", out);
// Pool allocator test.
    // PoolAllocator* pool = PoolAllocator::create(vmem, sizeof(Entity), alignof(Entity), 128);
    // defer(pool->destroy());

    // char temp_buf[20];
    
    // for (sz i = 0; i < 256; ++i)
    // {
    //     populate_buff_ascii(temp_buf, 20);
    //     auto* en = (Entity*)pool->allocate();
    //     en->init({temp_buf, 20}, rand_in_range(10, 99));
    //     en->dance();
    //     pool->free(en);
    // }

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
