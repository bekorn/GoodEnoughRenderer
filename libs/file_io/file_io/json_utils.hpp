#pragma once

#include <rapidjson/document.h>
#include <core/core.hpp>

namespace File::JSON
{
template<bool Const>
using Obj = rapidjson::GenericObject<Const, rapidjson::Value> const &;
using ConstObj = Obj<true>;
using MutObj = Obj<false>;
using Key = std::string_view const &;

template<bool Const>
inline u32 GetU32(Obj<Const> obj, Key key)
{
	return obj[key.data()].GetUint();
}
template<bool Const>
inline u32 GetU32(Obj<Const> obj, Key key, u32 def_value)
{
	auto member = obj.FindMember(key.data());
	if (member != obj.MemberEnd())
		return member->value.GetUint();
	else
		return def_value;
}
template<bool Const>
inline optional<u32> GetOptionalU32(Obj<Const> obj, Key key)
{
	auto member = obj.FindMember(key.data());
	if (member != obj.MemberEnd())
		return member->value.GetUint();
	else
		return nullopt;
}

template<bool Const>
inline i32 GetI32(Obj<Const> obj, Key key, i32 def_value)
{
	auto member = obj.FindMember(key.data());
	if (member != obj.MemberEnd())
		return member->value.GetInt();
	else
		return def_value;
}

template<bool Const>
inline std::string GetString(Obj<Const> obj, Key key, std::string const & def_value)
{
	auto member = obj.FindMember(key.data());
	if (member != obj.MemberEnd())
		return member->value.GetString();
	else
		return def_value;
}
template<bool Const>
inline optional<std::string> GetOptionalString(Obj<Const> obj, Key key)
{
	auto member = obj.FindMember(key.data());
	if (member != obj.MemberEnd())
		return member->value.GetString();
	else
		return nullopt;
}

template<bool Const>
inline bool GetBool(Obj<Const> obj, Key key, bool def_value)
{
	auto member = obj.FindMember(key.data());
	if (member != obj.MemberEnd())
		return member->value.GetBool();
	else
		return def_value;
}

template<bool Const>
inline f32 GetF32(Obj<Const> obj, Key key, f32 def_value)
{
	auto member = obj.FindMember(key.data());
	if (member != obj.MemberEnd())
		return member->value.GetFloat();
	else
		return def_value;
}

template<bool Const>
inline f32x3 GetF32x3(Obj<Const> obj, Key key, f32x3 const & def_value)
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

template<bool Const>
inline f32x4 GetF32x4(Obj<Const> obj, Key key, f32x4 const & def_value)
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

	template<bool Const>
	std::string get(Obj<Const> obj, Key key)
	{
		auto member = obj.FindMember(key.data());
		if (member != obj.MemberEnd())
			return fmt::format("{}{}:{}", prefix, idx++, member->value.GetString());
		else
			return fmt::format("{}{}", prefix, idx++);
	}
};
}