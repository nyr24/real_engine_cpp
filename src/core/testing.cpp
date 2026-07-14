#include "core/testing.hpp"

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

} // rg
