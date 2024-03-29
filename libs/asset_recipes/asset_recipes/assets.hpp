#pragma once

#include "descriptions.hpp"

#include <core/core.hpp>
#include <core/named.hpp>
#include <opengl/texture_2d.hpp>
#include <opengl/texture_cubemap.hpp>
#include <opengl/texture_3d.hpp>
#include <render/mesh.hpp>
#include <render/material.hpp>
#include <render/scene.hpp>

struct Assets
{
	Descriptions const & descriptions;

	// GL resources
	Managed<Geometry::Layout> vertex_layouts;
	Managed<GL::UniformBlock> uniform_blocks;
	Managed<GL::ShaderProgram> programs;
	Managed<GL::Texture2D> textures;
	Managed<GL::TextureCubemap> texture_cubemaps;
	Managed<GL::Texture3D> volumes;
	// Render resources
	Managed<Geometry::Primitive> primitives;
	Managed<unique_one<Render::IMaterial>> materials;
	Managed<Render::Mesh> meshes;
	// Scene resources
	Scene::Tree scene_tree;

	// For editing purposes
	Managed<GL::ShaderProgram> initial_program_interfaces;
	Managed<std::string> program_errors;

	explicit Assets(Descriptions const & descriptions) :
		descriptions(descriptions)
	{}

	COPY(Assets, delete)
	MOVE(Assets, delete)

	void init();

	void load_glsl_vertex_layout(Name const & name);
	void load_glsl_program(Name const & name);
	void load_glsl_uniform_block(Name const & name);
	void load_gltf(Name const & name);
	void load_texture(Name const & name);
	void load_cubemap(Name const & name);
	void load_envmap(Name const & name);
	// For editing purposes
	bool reload_glsl_program(Name const & name);
};