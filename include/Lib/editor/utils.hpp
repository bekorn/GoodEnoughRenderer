#pragma once

#include "core.hpp"

namespace Editor
{
// TODO(bekorn): as I understand, imgui already has a buffer to keep formatted strings, so this might be unnecessary
// ImGui + fmtlib utility (especially handy for tables)
extern fmt::memory_buffer _buffer;
extern std::back_insert_iterator<fmt::memory_buffer> _buffer_iter;
template<typename... Args>
void TextFMT(fmt::format_string<Args...> fmt, Args && ... args)
{
	_buffer.clear(), fmt::vformat_to(_buffer_iter, fmt, fmt::make_format_args(args...));
	ImGui::TextUnformatted(_buffer.begin(), _buffer.end());
};

}