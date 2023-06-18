#pragma once

#include <file_io/core.hpp>
#include <file_io/json_utils.hpp>

#include <asset_recipes/glsl/vertex_layout/load.hpp>

#include "recipes/attrib_layout.hpp"

struct Book
{
	std::filesystem::path const root;

	Managed<Geometry::Layout> attrib_layouts;

	Book(std::filesystem::path const & root) :
		root(root)
	{
		auto json_path = root / "assets.json";
		assert(exists(json_path), "assets.json is absent");

		using namespace rapidjson;
		using namespace File::JSON;

		Document document;
		document.Parse(File::LoadAsString(json_path).c_str());
		assert(not document.HasParseError(), "assets.json is invalid");


		if (auto const member = document.FindMember(AttribLayout::NAME); member != document.MemberEnd())
			for (auto const & item: member->value.GetArray())
			{
				auto [name, desc] = AttribLayout::Parse(item.GetObject());
				attrib_layouts.generate(name, desc);
			}
	}
};
