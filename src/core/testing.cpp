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

} // rg
