#pragma once

#include <file_io/core.hpp>
#include <file_io/json_utils.hpp>

#include <asset_recipes/attrib_layout/convert.hpp>

#include "recipes/gltf.hpp"

struct Book
{
	std::filesystem::path const assets_dir;
	std::filesystem::path const served_dir;

	Managed<Geometry::Layout> attrib_layouts;
	Managed<GLTF::Desc> gltf;

	Book(std::filesystem::path const & assets_dir) :
		assets_dir(assets_dir), served_dir(std::filesystem::path(assets_dir).concat("__served"))
	{
		auto json_path = assets_dir / "assets.json";
		assert(exists(json_path), "assets.json is absent");

		using namespace rapidjson;
		using namespace File::JSON;

		Document document;
		document.Parse(File::LoadAsString(json_path).c_str());
		assert(not document.HasParseError(), "assets.json is invalid");

		if (auto const member = document.FindMember(AttribLayout::ASSET_NAME); member != document.MemberEnd())
			for (auto const & item: member->value.GetArray())
			{
				auto [name, desc] = AttribLayout::Parse(item.GetObject(), assets_dir);
				attrib_layouts.generate(name, desc);
			}

		if (auto const member = document.FindMember(GLTF::NAME); member != document.MemberEnd())
			for (auto const & item: member->value.GetArray())
			{
				auto [name, desc] = GLTF::Parse(item.GetObject(), assets_dir);
				gltf.generate(name, desc);
			}
	}
};
