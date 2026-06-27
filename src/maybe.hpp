#ifndef _RG_MAYBE_HPP_
#define _RG_MAYBE_HPP_

// Maybe.

namespace rg
{

template <typename Type>
struct Maybe
{
    Type val;
    bool has_value;

    inline operator bool() { return has_value; }
};

template <typename Type>
inline Maybe<Type> maybe_create(Type val)
{
    return Maybe{ val, true };
}

template <typename Type>
inline Maybe<Type> maybe_create_empty()
{
    return Maybe<Type>{ .has_value = false };
}

} // rg

#endif // _RG_MAYBE_HPP_
