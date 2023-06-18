#include "attrib_layout.hpp"

Geometry::Key get_key(const char * name)
{
	using enum Geometry::Key::Common;
	if (0 == strcmp(name, "position")) return {POSITION, 0};
	if (0 == strcmp(name, "normal")) return {NORMAL, 0};
	if (0 == strcmp(name, "tangent")) return {TANGENT, 0};
	if (0 == strcmp(name, "texcoord")) return {TEXCOORD, 0};
	if (0 == strcmp(name, "color")) return {COLOR, 0};
	if (0 == strcmp(name, "joints")) return {JOINTS, 0};
	if (0 == strcmp(name, "weights")) return {WEIGHTS, 0};
	return {name, 0};
}

namespace AttribLayout
{
std::pair<Name, Geometry::Layout> Parse(File::JSON::JSONObj o)
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