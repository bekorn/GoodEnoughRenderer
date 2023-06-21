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
	if (name == "u8norm") return {U8NORM, 1};
	if (name == "u8normx2") return {U8NORM, 2};
	if (name == "u8normx3") return {U8NORM, 3};
	if (name == "u8normx4") return {U8NORM, 4};
	if (name == "u16norm") return {U16NORM, 1};
	if (name == "u16normx2") return {U16NORM, 2};
	if (name == "u16normx3") return {U16NORM, 3};
	if (name == "u16normx4") return {U16NORM, 4};
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