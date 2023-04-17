#pragma once

#include "core.hpp"
#include "vao.hpp"

namespace GL
{
inline VertexArray dummy_vao;

inline void init_globals()
{
	glCreateVertexArrays(1, &dummy_vao.id);
}

}