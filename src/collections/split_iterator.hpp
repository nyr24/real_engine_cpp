#ifndef _RG_SPLIT_ITERATOR_HPP_
#define _RG_SPLIT_ITERATOR_HPP_

#include "slice.hpp"

namespace rg
{

template<typename Type>
struct SplitIterator
{
    Slice<Type> view;
    Type splitter;
    sz pos;

    SplitIterator() = default;
    SplitIterator(Slice<Type> view_, Type splitter_);
    // Can return empty slice - this will mark the end of iteration loop.
    Slice<Type> next();
    void foreach(void(*fn)(Slice<Type>));
    inline bool is_at_end();
    inline void reset();
};

template<typename Type>
SplitIterator<Type>::SplitIterator(Slice<Type> view_, Type splitter_)
    : view{view_}, splitter{splitter_}, pos{0}
{
}

template<typename Type>
Slice<Type> SplitIterator<Type>::next()
{
    if (this->is_at_end()) return {};
    sz start = this->pos;
    sz curr = start;
    sz end = this->view.count;
    Type curr_val;

    for (; curr < end; ++curr)
    {
        if (this->view[curr] == this->splitter) break;
    }

    sz dist = curr - start;
    ASSERT_MSG(dist > 0, "Distance can't be zero");

    Slice<Type> res{ this->view + start, dist };
    this->pos += dist;
    return res;
}

template<typename Type>
void SplitIterator<Type>::foreach(void(*fn)(Slice<Type>))
{
    Slice<Type> seq;
    for (;;)
    {
        seq = this->next();
        if (seq.is_empty()) return;
        fn(seq);
    } 
}

template<typename Type>
inline bool SplitIterator<Type>::is_at_end()
{
    return this->pos >= this->view.count;
}

template<typename Type>
inline void SplitIterator<Type>::reset()
{
    this->pos = 0;
}

} // rg

#endif // _RG_SPLIT_ITERATOR_HPP_
