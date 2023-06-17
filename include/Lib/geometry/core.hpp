#pragma once

#include <core/core.hpp>
#include <core/intrinsics.hpp>
#include <core/named.hpp>

namespace Geometry
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
		if (holds_alternative<std::string>(name))
			return get<std::string>(name).data();

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
		assert_enum_out_of_range();
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
	u8 dimension = 0;

	bool operator==(Type const & other) const = default;

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
		assert_enum_out_of_range();
	}

	u8 vector_size() const
	{ return size() * dimension; }

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
		assert_enum_out_of_range();
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
		assert_enum_out_of_range();
	}
};

constexpr auto const ATTRIBUTE_COUNT = 16;

struct Attribute
{
	Key key;
	Type type;

	u8 location;
	bool is_per_patch;

	u8 group;

	bool is_used() const
	{ return type.dimension != 0; }
};

struct Layout
{
	array<Attribute, ATTRIBUTE_COUNT> attributes;

	decltype(auto) operator[](usize idx) { return attributes[idx]; }
	decltype(auto) begin() const { return attributes.begin(); }
	decltype(auto) end() const { return attributes.end(); }
};

struct LayoutMask
{
	array<bool, ATTRIBUTE_COUNT> is_active;
};

struct Data
{
	array<ByteBuffer, ATTRIBUTE_COUNT> buffers;

	decltype(auto) operator[](usize idx) { return buffers[idx]; }
	decltype(auto) begin() const { return buffers.begin(); }
	decltype(auto) end() const { return buffers.end(); }
};

struct Primitive
{
	const Geometry::Layout * layout;
	Geometry::Data data;
	vector<u32> indices;

	CTOR(Primitive, default);
	COPY(Primitive, delete);
	MOVE(Primitive, default);

	ByteBuffer & get_buffer(Geometry::Key const & key)
	{
		auto idx = std::ranges::find(layout->attributes, key, &Attribute::key) - layout->attributes.begin();
		assert(idx < ATTRIBUTE_COUNT);
		return data.buffers[idx];
	}
};
}

template<>
struct fmt::formatter<Geometry::Key>
{
	constexpr auto parse(format_parse_context & ctx) -> decltype(ctx.begin())
	{ return ctx.end(); }

	template<typename FormatContext>
	auto format(Geometry::Key key, FormatContext & ctx) const
	{ return fmt::format_to(ctx.out(), "{}:{}", key.name_to_string(), key.layer); }
};

template<>
struct fmt::formatter<Geometry::Type>
{
	constexpr auto parse(format_parse_context & ctx) -> decltype(ctx.begin())
	{ return ctx.end(); }

	template<typename FormatContext>
	auto format(Geometry::Type type, FormatContext & ctx) const
	{ return fmt::format_to(ctx.out(), "{}", type.value_to_string()); }
};
