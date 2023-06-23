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

namespace AttribLayout
{
std::pair<Name, Geometry::Layout> Parse(File::JSON::JSONObj o, std::filesystem::path const & root_dir)
{
	Geometry::Layout layout;
	u8 location = 0;
	for (auto const & member: o["attribs"].GetObject())
	{
		auto const & name = member.name.GetString();
		auto const & attrib = member.value.GetObject();
		layout[location] = Geometry::Attribute{
			.key = get_key(name),
			.vec = {
				.type = {Geometry::Type::from_string(attrib["type"].GetString())},
				.dimension = u8(attrib["size"].GetUint())
			},
			.location = location,
			.is_per_patch = attrib["per_patch"].GetBool(),
			.group = u8(attrib["group"].GetUint()),
		};

		location++;
	}

	fmt::print("Layout {}:\n{}\n", o["name"].GetString(), layout);

	return {
		o["name"].GetString(),
		{layout}
	};
}
}