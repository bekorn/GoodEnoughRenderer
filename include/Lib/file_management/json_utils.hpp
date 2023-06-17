#pragma once

#include <core/core.hpp>

namespace File::JSON
{
using JSONObj = rapidjson::Document::ConstObject const &;
using Key = std::string_view const &;

inline u32 GetU32(JSONObj obj, Key key)
{
	return obj[key.data()].GetUint();
}

inline i32 GetI32(JSONObj obj, Key key, i32 def_value)
{
	auto member = obj.FindMember(key.data());
	if (member != obj.MemberEnd())
		return member->value.GetInt();
	else
		return def_value;
}

inline u32 GetU32(JSONObj obj, Key key, u32 def_value)
{
	auto member = obj.FindMember(key.data());
	if (member != obj.MemberEnd())
		return member->value.GetUint();
	else
		return def_value;
}

inline optional<u32> GetOptionalU32(JSONObj obj, Key key)
{
	auto member = obj.FindMember(key.data());
	if (member != obj.MemberEnd())
		return member->value.GetUint();
	else
		return nullopt;
}

inline std::string GetString(JSONObj obj, Key key, std::string const & def_value)
{
	auto member = obj.FindMember(key.data());
	if (member != obj.MemberEnd())
		return member->value.GetString();
	else
		return def_value;
}

inline bool GetBool(JSONObj obj, Key key, bool def_value)
{
	auto member = obj.FindMember(key.data());
	if (member != obj.MemberEnd())
		return member->value.GetBool();
	else
		return def_value;
}

inline f32 GetF32(JSONObj obj, Key key, f32 def_value)
{
	auto member = obj.FindMember(key.data());
	if (member != obj.MemberEnd())
		return member->value.GetFloat();
	else
		return def_value;
}

inline f32x3 GetF32x3(JSONObj obj, Key key, f32x3 def_value)
{
	auto member = obj.FindMember(key.data());
	if (member != obj.MemberEnd())
	{
		auto const & arr = member->value.GetArray();
		f32x3 val;
		for (auto i = 0; i < 3; ++i)
			val[i] = arr[i].GetFloat();
		return val;
	}
	else
		return def_value;
}

inline f32x4 GetF32x4(JSONObj obj, Key key, f32x4 def_value)
{
	auto member = obj.FindMember(key.data());
	if (member != obj.MemberEnd())
	{
		auto const & arr = member->value.GetArray();
		f32x4 val;
		for (auto i = 0; i < 4; ++i)
			val[i] = arr[i].GetFloat();
		return val;
	}
	else
		return def_value;
}

struct NameGenerator
{
	std::string const prefix;
	u64 idx = 0;

	std::string get(JSONObj obj, Key key)
	{
		auto member = obj.FindMember(key.data());
		if (member != obj.MemberEnd())
			return fmt::format("{}{}:{}", prefix, idx++, member->value.GetString());
		else
			return fmt::format("{}{}", prefix, idx++);
	}
};
}