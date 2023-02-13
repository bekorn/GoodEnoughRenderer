#pragma once

#include "Lib/core/core.hpp"
#include "Lib/core/intrinsics.hpp"
#include "Lib/core/named.hpp"

namespace Geometry
{
namespace Attribute
{
struct Key
{
	enum Common : u8
	{
		POSITION,
		NORMAL,
		TANGENT,
		TEXCOORD,
		COLOR,
		JOINTS,
		WEIGHTS,
	};

	variant<Common, std::string> name;
	u8 layer;

	bool operator==(Key const & other) const
	{
		return layer == other.layer and name == other.name;
	}

	struct Hasher
	{
		usize operator()(Key const & key) const
		{
			// TODO(bekorn): is this a good hash???
			return std::hash<decltype(key.layer)>{}(key.layer) ^ std::hash<decltype(key.name)>{}(key.name);
		}
	};

	const char * name_to_string() const
	{
		if (holds_alternative<Common>(name))
			switch (get<Common>(name))
			{
			case POSITION: return "POSITION";
			case NORMAL: return "NORMAL";
			case TANGENT: return "TANGENT";
			case TEXCOORD: return "TEXCOORD";
			case COLOR: return "COLOR";
			case JOINTS: return "JOINTS";
			case WEIGHTS: return "WEIGHTS";
			}
		else
			return get<std::string>(name).data();
	}
};

struct Type
{
	enum Value : u8
	{
		F32,
		I8, I16, I32, I8NORM, I16NORM, I32NORM,
		U8, U16, U32, U8NORM, U16NORM, U32NORM,
	};

	Value value;

	Type(Value value) :
		value(value)
	{}

	operator Value() const
	{ return value; }

	u8 size() const
	{
		switch (value)
		{
		case I8:
		case U8:
		case I8NORM:
		case U8NORM: return 1;
		case I16:
		case U16:
		case I16NORM:
		case U16NORM: return 2;
		case F32:
		case I32:
		case U32:
		case I32NORM:
		case U32NORM: return 4;
		}
	}

	bool is_normalized() const
	{
		switch (value)
		{
		case I8NORM:
		case U8NORM:
		case I16NORM:
		case U16NORM:
		case I32NORM:
		case U32NORM: return true;
		case I8:
		case U8:
		case I16:
		case U16:
		case F32:
		case I32:
		case U32: return false;
		}
	}

	const char * value_to_string() const
	{
		using enum Type::Value;
		switch (value)
		{
		case F32: return "F32";
		case I8: return "I8";
		case I16: return "I16";
		case I32: return "I32";
		case I8NORM: return "I8NORM";
		case I16NORM: return "I16NORM";
		case I32NORM: return "I32NORM";
		case U8: return "U8";
		case U16: return "U16";
		case U32: return "U32";
		case U8NORM: return "U8NORM";
		case U16NORM: return "U16NORM";
		case U32NORM: return "U32NORM";
		}
	}
};

struct Data
{
	Type type;
	u8 dimension;

	ByteBuffer buffer;
};
}

struct Primitive
{
	std::unordered_map<Attribute::Key, Attribute::Data, Attribute::Key::Hasher> attributes;
	vector<u32> indices;

	CTOR(Primitive, default);
	COPY(Primitive, delete);
	MOVE(Primitive, default);
};
}

template<>
struct fmt::formatter<Geometry::Attribute::Key>
{
	constexpr auto parse(format_parse_context & ctx) -> decltype(ctx.begin())
	{ return ctx.end(); }

	template<typename FormatContext>
	auto format(Geometry::Attribute::Key key, FormatContext & ctx) const
	{ return fmt::format_to(ctx.out(), "{}:{}", key.name_to_string(), key.layer); }
};

template<>
struct fmt::formatter<Geometry::Attribute::Type>
{
	constexpr auto parse(format_parse_context & ctx) -> decltype(ctx.begin())
	{ return ctx.end(); }

	template<typename FormatContext>
	auto format(Geometry::Attribute::Type type, FormatContext & ctx) const
	{ return fmt::format_to(ctx.out(), "{}", type.value_to_string()); }
};
