#ifndef _RG_GEOMETRY_HPP_
#define _RG_GEOMETRY_HPP_

#include "core/math.hpp"

namespace rg
{
    struct Vertex
    {
        Vec3 pos;
        Vec3 normal;
        Vec2 tex_coord;
    };
} // rg

#endif // _RG_GEOMETRY_HPP_ 
