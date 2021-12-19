#pragma once

#include "Lib/core/core.hpp"

#include "core.hpp"

namespace File
{
	namespace JSON
	{
		using JSONObj = rapidjson::Document::ConstObject const &;
		using Key = std::string_view const &;

		u32 GetU32(JSONObj obj, Key key)
		{
			return obj[key.data()].GetUint();
		}

		u32 GetU32(JSONObj obj, Key key, u32 def_value)
		{
			auto member = obj.FindMember(key.data());
			if (member != obj.MemberEnd())
				return member->value.GetUint();
			else
				return def_value;
		}

		optional<u32> GetOptionalU32(JSONObj obj, Key key)
		{
			auto member = obj.FindMember(key.data());
			if (member != obj.MemberEnd())
				return member->value.GetUint();
			else
				return nullopt;
		}

		std::string GetString(JSONObj obj, Key key, std::string const & def_value)
		{
			auto member = obj.FindMember(key.data());
			if (member != obj.MemberEnd())
				return member->value.GetString();
			else
				return def_value;
		}

		bool GetBool(JSONObj obj, Key key, bool def_value)
		{
			auto member = obj.FindMember(key.data());
			if (member != obj.MemberEnd())
				return member->value.GetBool();
			else
				return def_value;
		}

		f32 GetF32(JSONObj obj, Key key, f32 def_value)
		{
			auto member = obj.FindMember(key.data());
			if (member != obj.MemberEnd())
				return member->value.GetFloat();
			else
				return def_value;
		}

		f32x3 GetF32x3(JSONObj obj, Key key, f32x3 def_value)
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

		f32x4 GetF32x4(JSONObj obj, Key key, f32x4 def_value)
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
	}
}