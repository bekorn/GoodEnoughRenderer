#pragma once

#include <file_io/core.hpp>
#include <file_io/json_utils.hpp>

#include <asset_recipes/attrib_layout/convert.hpp>

#include "gltf.hpp"

struct Book
{
	File::Path const assets_abs_dir_path;
	File::Path const served_abs_dir_path;

	Managed<Geometry::Layout> attrib_layouts;
	Managed<GLTF::Desc> gltf;

	Book(File::Path const & assets_abs_dir_path) :
		assets_abs_dir_path(assets_abs_dir_path),
		served_abs_dir_path(File::Path(assets_abs_dir_path) += "__served")
	{
		auto json_abs_file_path = assets_abs_dir_path / "assets.json";
		assert(exists(json_abs_file_path), "assets.json is absent");

		using namespace rapidjson;
		using namespace File::JSON;

		Document document;
		document.Parse(File::LoadAsString(json_abs_file_path).c_str());
		assert(not document.HasParseError(), "assets.json is invalid");

		if (auto const member = document.FindMember(AttribLayout::ASSET_NAME); member != document.MemberEnd())
			for (auto const & item: member->value.GetArray())
			{
				auto [name, desc] = AttribLayout::Parse(item.GetObject(), assets_abs_dir_path);
				attrib_layouts.generate(name, desc);
			}

		if (auto const member = document.FindMember(GLTF::ASSET_NAME); member != document.MemberEnd())
			for (auto const & item: member->value.GetArray())
			{
				auto [name, desc] = GLTF::Parse(item.GetObject(), assets_abs_dir_path);
				gltf.generate(name, desc);
			}
	}
};
