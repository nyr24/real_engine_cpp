#include "core/testing.hpp"
#include "collections/thread_pool.hpp"
#include "core/io.hpp"
#include "core/allocators.hpp"
#include "core/context.hpp"

namespace rg
{

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

bool is_prime(int x)
{
    if (x < 2) return false;
    for (s32 i = 2; i < x; ++i)
    {
        if (x % i == 0)
		{
            return false;
		}
    }
    return true;
}

s32 next_prime(s32 x)
{
    for (; !is_prime(x); ++x)
	{}
    return x;
}

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

// Thread pool.

// Threadpool and Io.
void thread_pool_test()
{
    ThreadPool<4, 16> tpool;
    tpool.init();
    Arena* arena = get_temp_allocator();
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
}

} // rg
