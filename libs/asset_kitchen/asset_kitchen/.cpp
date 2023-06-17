#include "assets.hpp"
#include "descriptions.hpp"
#include "glsl/vertex_layout/convert.hpp"
#include "glsl/program/convert.hpp"
#include "glsl/uniform_block/convert.hpp"
#include "gltf/convert.hpp"
#include "texture/convert.hpp"
#include "cubemap/convert.hpp"
#include "envmap/convert.hpp"

void Descriptions::init(std::filesystem::path const & project_root)
{
	root = project_root;

	auto asset_descriptions_path = project_root / "assets.json";
	if (std::filesystem::exists(asset_descriptions_path))
	{
		using namespace rapidjson;
		using namespace File::JSON;

		Document document;
		document.Parse(File::LoadAsString(asset_descriptions_path).c_str());
		assert(document.IsObject(), "assets.json is invalid");


		if (auto const member = document.FindMember("glsl_vertex_layout"); member != document.MemberEnd())
			for (auto const & item: member->value.GetArray())
			{
				auto [name, desc] = GLSL::VertexLayout::Parse(item.GetObject(), project_root);
				vertex_layout.generate(name, desc);
			}

		if (auto const member = document.FindMember("glsl_uniform_block"); member != document.MemberEnd())
			for (auto const & item: member->value.GetArray())
			{
				auto [name, desc] = GLSL::UniformBlock::Parse(item.GetObject(), project_root);
				uniform_block.generate(name, desc);
			}

		if (auto const member = document.FindMember("glsl_program"); member != document.MemberEnd())
			for (auto const & item: member->value.GetArray())
			{
				auto [name, desc] = GLSL::Program::Parse(item.GetObject(), project_root);
				glsl.generate(name, desc);
			}

		if (auto const member = document.FindMember("gltf"); member != document.MemberEnd())
			for (auto const & item: member->value.GetArray())
			{
				auto [name, desc] = GLTF::Parse(item.GetObject(), project_root);
				gltf.generate(name, desc);
			}

		if (auto const member = document.FindMember("texture"); member != document.MemberEnd())
			for (auto const & item: member->value.GetArray())
			{
				auto [name, desc] = Texture::Parse(item.GetObject(), project_root);
				texture.generate(name, desc);
			}

		if (auto const member = document.FindMember("cubemap"); member != document.MemberEnd())
			for (auto const & item: member->value.GetArray())
			{
				auto [name, desc] = Cubemap::Parse(item.GetObject(), project_root);
				cubemap.generate(name, desc);
			}

		if (auto const member = document.FindMember("envmap"); member != document.MemberEnd())
			for (auto const & item: member->value.GetArray())
			{
				auto [name, desc] = Envmap::Parse(item.GetObject(), project_root);
				envmap.generate(name, desc);
			}
	}
}

void Assets::init()
{
	for (auto const & [name, _] : descriptions.vertex_layout)
		load_glsl_vertex_layout(name);

	for (auto const & [name, _] : descriptions.uniform_block)
		load_glsl_uniform_block(name);
	// TODO(bekorn): currently some uniform buffers are populated outside of render functions
	//  therefore can't be stopped with should_game_render. However there might be a way to gracefully resolve this.
	if (not program_errors.empty())
		std::exit(1);

	for (auto const & [name, _] : descriptions.glsl)
		load_glsl_program(name);

	for (auto const & [name, _] : descriptions.gltf)
		load_gltf(name);

	for (auto const & [name, _] : descriptions.texture)
		load_texture(name);

	for (auto const & [name, _] : descriptions.cubemap)
		load_cubemap(name);

	for (auto const & [name, _] : descriptions.envmap)
		load_envmap(name);
}

void Assets::load_glsl_vertex_layout(Name const & name)
{
	auto const layout_data = GLSL::VertexLayout::Load(descriptions.vertex_layout.get(name));
	vertex_layouts.generate(name, move(layout_data));
}

void Assets::load_glsl_program(Name const & name)
{
	auto const loaded_data = GLSL::Program::Load(descriptions.glsl.get(name));

	if (auto expected = GLSL::Program::Convert(loaded_data, vertex_layouts))
	{
		programs.generate(name, expected.into_result());
	}
	else
	{
		auto error = expected.into_error();
		fmt::print(stderr, "Failed to load GLSL Program {}. Error: {}", name.string, error);
		programs.generate(name); // so it shows up in the editor
		program_errors.generate(name, error);
	}
}

void Assets::load_glsl_uniform_block(Name const & name)
{
	auto const loaded_data = GLSL::UniformBlock::Load(descriptions.uniform_block.get(name));

	if (auto expected = GLSL::UniformBlock::Convert(loaded_data))
	{
		uniform_blocks.generate(name, expected.into_result());
		program_errors.erase(name);
	}
	else
	{
		auto error = expected.into_error();
		fmt::print(stderr, "Failed to load Uniform Block {}. Error: {}", name.string, error);
		program_errors.generate(name, error);
	}
}

void Assets::load_gltf(Name const & name)
{
	auto const gltf_data = GLTF::Load(descriptions.gltf.get(name));
	GLTF::Convert(gltf_data, textures, materials, primitives, meshes, scene_tree, vertex_layouts);
}

void Assets::load_texture(const Name & name)
{
	auto texture_data = Texture::Load(descriptions.texture.get(name));
	textures.generate(name, move(Texture::Convert(texture_data)));
}

void Assets::load_cubemap(Name const & name)
{
	auto cubemap_data = Cubemap::Load(descriptions.cubemap.get(name));
	texture_cubemaps.generate(name, move(Cubemap::Convert(cubemap_data)));
}

void Assets::load_envmap(Name const & name)
{
	auto envmap_data = Envmap::Load(descriptions.envmap.get(name));
	Envmap::Convert(envmap_data, name, texture_cubemaps);
}

template<std::ranges::range Range>
bool IsSubsetOf(Range const & l, Range const & r)
{
	auto l_iter = l.begin();
	auto r_iter = r.begin();
	auto const l_end = l.end();
	auto const r_end = r.end();

	for (; l_iter != l_end and r_iter != r_end; ++l_iter)
		if (GL::SameMappingType(*l_iter, *r_iter))
			++r_iter;

	if (r_iter == r_end) // all items matched
		return true;
	return false;
}

bool Assets::reload_glsl_program(Name const & name)
{
	auto const loaded_data = GLSL::Program::Load(descriptions.glsl.get(name));

	if (auto expected = GLSL::Program::Convert(loaded_data, vertex_layouts))
	{
		auto new_program = expected.into_result();

		if (programs.get(name).id != 0)
		{
			// the first time reloading this program
			if (auto iter = initial_program_interfaces.find(name); iter == initial_program_interfaces.end())
			{
				auto & program = programs.get(name);
				auto & interface = initial_program_interfaces.generate(name).data;
				interface.uniform_mappings = program.uniform_mappings;
				interface.uniform_block_mappings = program.uniform_block_mappings;
				interface.storage_block_mappings = program.storage_block_mappings;
			}

			auto & interface = initial_program_interfaces.get(name);

			/*
			// check if the new program's interface are a subset of the old, prevent shader interface additions on runtime
			char const * missmatch = nullptr;
			if (not IsSubsetOf(interface.attribute_mappings, new_program.attribute_mappings))
				missmatch = "attribute";
			else if (not IsSubsetOf(interface.uniform_mappings, new_program.uniform_mappings))
				missmatch = "uniform";
			else if (not IsSubsetOf(interface.uniform_block_mappings, new_program.uniform_block_mappings))
				missmatch = "uniform_block";
			else if (not IsSubsetOf(interface.storage_block_mappings, new_program.storage_block_mappings))
				missmatch = "storage_block";

			if (missmatch != nullptr)
			{
				auto error = fmt::format(
					"Failed to reload GLSL Program {}: {} mappings is not a subset of the previous\n",
					name.string, missmatch
				);
				// TODO(bekorn): logging only because the asset_kitchen is not stable enough, should be removed later
				fmt::print(stderr, "{}", error);
				program_errors.get_or_generate(name) = error;
				return false;
			}
			*/
		}

		programs.get(name) = move(new_program);
		program_errors.erase(name);
		return true;
	}
	else
	{
		auto error = expected.into_error();
		// TODO(bekorn): logging only because the asset_kitchen is not stable enough, should be removed later
		fmt::print(stderr, "Failed to reload GLSL Program {}. Error: {}\n", name.string, error);
		program_errors.get_or_generate(name) = error;
		return false;
	}
}
