#pragma once

#include "core.hpp"

// TODO(bekorn): this dependency should be reversed, opengl should be contained in itself,
//  geometry should build on top of opengl (or any other API later on)
#include "Lib/geometry/core.hpp"

namespace GL
{
	struct AttributeMapping
	{
		u32 location;
		GLenum glsl_type; // just for debug purposes
		bool per_patch;
		Geometry::Attribute::Key key;
	};

	struct UniformMapping
	{
		u32 location;
		GLenum glsl_type; // just for debug purposes
		std::string key;
	};

	struct UniformBlockMapping
	{
		u32 location;
		u32 data_size;
		std::string key;

		struct Variable
		{
			u32 offset;
			GLenum glsl_type; // just for debug purposes
			std::string key;
		};
		vector<Variable> variables;
	};

	struct StorageBlockMapping
	{
		u32 location;
		u32 data_size;
		std::string key;

		struct Variable
		{
			u32 offset;
			GLenum glsl_type; // just for debug purposes
			std::string key;
		};
		vector<Variable> variables;
	};

	// results should be cached for better performance
	auto & GetMapping(std::ranges::range auto const & mappings, auto const & key)
	{
		for (auto & mapping: mappings)
			if (key == mapping.key)
				return mapping;

		assert(("key not found", false));
	}

	// results should be cached for better performance
	GLuint GetLocation(std::ranges::range auto const & mappings, auto const & key)
	{ return GetMapping(mappings, key).location; }
}