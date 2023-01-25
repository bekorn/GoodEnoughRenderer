#include "utils.hpp"

namespace Editor
{
fmt::memory_buffer _buffer;
std::back_insert_iterator<fmt::memory_buffer> _buffer_iter = std::back_inserter(_buffer);

}