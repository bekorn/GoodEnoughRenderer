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

// only difference is the default uv values, OpenGL assumes uv(0,0) is on bottom-left
inline void ImageGL(
	ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 1), const ImVec2& uv1 = ImVec2(1, 0),
	const ImVec4& tint_col = ImVec4(1,1,1,1), const ImVec4& border_col = ImVec4(0,0,0,0)
)
{ ImGui::Image(user_texture_id ,size ,uv0 ,uv1 ,tint_col ,border_col); }

}