#ifndef _RG_TESTING_HPP_
#define _RG_TESTING_HPP_

#include "core/basic.hpp"
#include "collections/string.hpp"

namespace rg
{

s32 rand_in_range(s32 start, s32 end);
void populate_buff_ascii(char* buff, sz capacity);
void print_buff_ascii(char* buff, sz capacity);

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

} // rg

#endif // _RG_TESTING_HPP_
