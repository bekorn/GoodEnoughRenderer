#include "load.hpp"
#include "convert.hpp"

Geometry::Key get_key(std::string_view name)
{
	using enum Geometry::Key::Common;
	if (name == "position") return {POSITION, 0};
	if (name == "normal") return {NORMAL, 0};
	if (name == "tangent") return {TANGENT, 0};
	if (name == "texcoord") return {TEXCOORD, 0};
	if (name == "color") return {COLOR, 0};
	if (name == "joints") return {JOINTS, 0};
	if (name == "weights") return {WEIGHTS, 0};
	return {std::string(name), 0};
}
Geometry::Vector get_type(std::string_view name)
{
	using enum Geometry::Type::Value;
	if (name == "f32") return {F32, 1};
	if (name == "f32x2") return {F32, 2};
	if (name == "f32x3") return {F32, 3};
	if (name == "f32x4") return {F32, 4};

	if (name == "u8") return {U8, 1};
	if (name == "u8x2") return {U8, 2};
	if (name == "u8x3") return {U8, 3};
	if (name == "u8x4") return {U8, 4};
	if (name == "u16") return {U16, 1};
	if (name == "u16x2") return {U16, 2};
	if (name == "u16x3") return {U16, 3};
	if (name == "u16x4") return {U16, 4};

	if (name == "u8norm") return {U8NORM, 1};
	if (name == "u8normx2") return {U8NORM, 2};
	if (name == "u8normx3") return {U8NORM, 3};
	if (name == "u8normx4") return {U8NORM, 4};
	if (name == "u16norm") return {U16NORM, 1};
	if (name == "u16normx2") return {U16NORM, 2};
	if (name == "u16normx3") return {U16NORM, 3};
	if (name == "u16normx4") return {U16NORM, 4};

	if (name == "i8") return {I8, 1};
	if (name == "i8x2") return {I8, 2};
	if (name == "i8x3") return {I8, 3};
	if (name == "i8x4") return {I8, 4};
	if (name == "i16") return {I16, 1};
	if (name == "i16x2") return {I16, 2};
	if (name == "i16x3") return {I16, 3};
	if (name == "i16x4") return {I16, 4};

	if (name == "i8norm") return {I8NORM, 1};
	if (name == "i8normx2") return {I8NORM, 2};
	if (name == "i8normx3") return {I8NORM, 3};
	if (name == "i8normx4") return {I8NORM, 4};
	if (name == "i16norm") return {I16NORM, 1};
	if (name == "i16normx2") return {I16NORM, 2};
	if (name == "i16normx3") return {I16NORM, 3};
	if (name == "i16normx4") return {I16NORM, 4};

	assert_failure("unknown vertex attribute");
}

namespace GLSL::VertexLayout
{
Geometry::Layout Load(Desc const & desc)
{
	return desc.attributes;
}

std::pair<Name, Desc> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir)
{
	fmt::print("Parsing layout {}\n", o["name"].GetString());

	Geometry::Layout layout;
	u8 location = 0;
	u8 group_idx = 0;
	for (auto const & group : o["groups"].GetArray())
	{
		fmt::print("Group {} has {} attributes\n", group_idx, group.MemberCount());
		for (auto const & [name, type] : group.GetObject())
		{
			fmt::print("\tAttribute {} is {}\n", name.GetString(), type.GetString());
			layout[location] = Geometry::Attribute{
				.key = get_key(name.GetString()),
				.vec = get_type(type.GetString()),

				.location = location,
				.is_per_patch = false,

				.group = group_idx,
			};
			location++;
		}
		group_idx++;
	}

	return {
		o["name"].GetString(),
		{layout}
	};
}
}