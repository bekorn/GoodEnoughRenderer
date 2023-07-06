#pragma once
#pragma message("-- read ASSET/GLTF/load.Hpp --")

#include <core/core.hpp>
#include <core/named.hpp>

namespace GLTF
{
// Spec: https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.pdf

using ::ByteBuffer;

struct Image
{
	ByteBuffer data;
	i32x2 dimensions;
	i32 channels;
	bool is_sRGB;
};

struct Sampler
{
	u32 min_filter;
	u32 mag_filter;
	u32 wrap_s;
	u32 wrap_t;
};
// TODO(bekorn): read about OpenGL Sampler Objects
static Sampler constexpr SamplerDefault{
	.min_filter = 9729, // LINEAR
	.mag_filter = 9729, // LINEAR
	.wrap_s = 10497, // REPEAT
	.wrap_t = 10497 // REPEAT
};

struct Texture
{
	std::string name;
	optional<u32> image_index;
	optional<u32> sampler_index;
};

// Equivalent of a draw call
struct Primitive
{
	std::string name;
	optional<u32> material_index;

	u32 vertex_buffer_offset, vertex_count;
	u32 index_buffer_offset, index_count;
};

struct Mesh
{
	std::string name;
	vector<Primitive> primitives;
};

struct Material
{
	std::string name;

	struct TexInfo
	{
		u32 texture_index;
		u32 texcoord_index; // which TEXCOORD_n attribute to use
	};

	struct PbrMetallicRoughness
	{
		f32x4 base_color_factor;
		optional<TexInfo> base_color_texture;
		f32 metallic_factor;
		f32 roughness_factor;
		optional<TexInfo> metallic_roughness_texture;
	};
	optional<PbrMetallicRoughness> pbr_metallic_roughness;

	optional<TexInfo> normal_texture;
	f32 normal_texture_scale;

	optional<TexInfo> occlusion_texture;
	f32 occlusion_texture_strength;

	optional<TexInfo> emissive_texture;
	f32x3 emissive_factor;

	std::string alpha_mode;
	f32 alpha_cutoff;

	bool double_sided;
};

struct Node
{
	std::string name;
	f32x3 translation;
	f32quat rotation;
	f32x3 scale;
	optional<u32> mesh_index;
	vector<u32> child_indices;
};

struct Scene
{
	std::string name;
	vector<u32> node_indices;
};

struct LoadedData
{
	ByteBuffer geometry_buffer;
	ByteBuffer image_buffer;

	vector<Image> images;
	vector<Sampler> samplers;
	vector<Texture> textures;

	vector<Mesh> meshes;
	optional<Name> layout_name;
	vector<Material> materials;

	vector<Node> nodes;
	Scene scene;
};

auto const pbrMetallicRoughness_program_name = "gltf_pbrMetallicRoughness"_name;

struct Desc
{
	std::string name;
	std::filesystem::path path;
	optional<Name> layout_name;
};

LoadedData Load(Desc const & desc);
}