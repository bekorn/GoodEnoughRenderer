#include "load.hpp"
#include "convert.hpp"

Geometry::Attribute::Key get_key(const char * name)
{
	if (0 == strcmp(name, "position")) return {Geometry::Attribute::Key::POSITION, 0};
	if (0 == strcmp(name, "normal")) return {Geometry::Attribute::Key::NORMAL, 0};
	if (0 == strcmp(name, "tangent")) return {Geometry::Attribute::Key::TANGENT, 0};
	if (0 == strcmp(name, "texcoord")) return {Geometry::Attribute::Key::TEXCOORD, 0};
	if (0 == strcmp(name, "color")) return {Geometry::Attribute::Key::COLOR, 0};
	if (0 == strcmp(name, "joints")) return {Geometry::Attribute::Key::JOINTS, 0};
	if (0 == strcmp(name, "weights")) return {Geometry::Attribute::Key::WEIGHTS, 0};
	return {name, 0};
}
Geometry::Attribute::Type get_type(const char * name)
{
	if (0 == strcmp(name, "f32")) return {Geometry::Attribute::Type::F32, 1};
	if (0 == strcmp(name, "f32x2")) return {Geometry::Attribute::Type::F32, 2};
	if (0 == strcmp(name, "f32x3")) return {Geometry::Attribute::Type::F32, 3};
	if (0 == strcmp(name, "f32x4")) return {Geometry::Attribute::Type::F32, 4};
	if (0 == strcmp(name, "u8norm")) return {Geometry::Attribute::Type::U8NORM, 1};
	if (0 == strcmp(name, "u8normx2")) return {Geometry::Attribute::Type::U8NORM, 2};
	if (0 == strcmp(name, "u8normx3")) return {Geometry::Attribute::Type::U8NORM, 3};
	if (0 == strcmp(name, "u8normx4")) return {Geometry::Attribute::Type::U8NORM, 4};
	assert_failure("unknown vertex attribute type");
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
	u8 binding_idx = 0;
	u8 group_idx = 0;
	for (auto const & group : o["groups"].GetArray())
	{
		fmt::print("Group {} has {} attributes\n", group_idx, group.MemberCount());
		for (auto const & [name, type] : group.GetObject())
		{
			fmt::print("\tAttribute {} is {}\n", name.GetString(), type.GetString());

			if (layout.try_emplace(
				get_key(name.GetString()),
				Geometry::Attribute::Layout{
					.type = get_type(type.GetString()), .group_idx = group_idx, .location = binding_idx
				}
			).second == false)
				fmt::print(stderr, "\t!! failed to emplace vertex layout. name={}\n", name.GetString());
			binding_idx++;
		}
		group_idx++;
	}

	return {
		o["name"].GetString(),
		{layout}
	};
}
}