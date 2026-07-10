# Reflections on c++ features I've encountered so far.

const T& - useful when you dont want to modify the value, but not sure how to pass it more effective, by value or by reference.
  compiler can optimize const references, replacing them by values.

T& - should be never used, because can't be seen at call site, pointers are better.

T&& - rarely useful.

Constructors - not sure, idea to combine allocation with initialization is very dumb and stupid,
  but standardized way of initialization function across language is useful, but it should be something like @operator("init")
  which works on already allocated objects.

Copy constructors / copy assignment operators - useful, but should never do deep copy of heap-allocated object, explicit 'clone' function is much better
 should just do shallow copy, if we don't have a destructor we don't care about pointing to the same memory multiple times.

Move constructors - useful standardized way of moving things, but rarely used.

Exceptions - total crap.

Destructors - absolute cancer, nobody should ever try to use them, they only create problems,
  its also the reason why people want to move ownership by 1000 times in an hour.

interfaces through virtual methods - bad, makes hidden allocation on the heap somewhere, creates hidden member for struct, which you can overwrite sometimes,
  can't be used without 'new' operator, better to make manuall, explicit c-style virtual tables, containing function pointers allocated in global memory.

new / delete - bad because you can't provide your own custom allocator, you don't decide if memory will be zeroed or not,
  2 versions - plain and array one - user forced to remember this for some reason.

std:: - not even a single container is good enough, everything is a total unusable RAII crap.

private - useful for hiding functions, but rarely for the data.

auto [handle, is_ok] = open_file("blabla.txt"); - finally, some cool feature!, no need to pass ugly output parameters anymore, or checking error codes.
