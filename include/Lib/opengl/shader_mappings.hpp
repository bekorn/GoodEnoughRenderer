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

	inline bool operator==(AttributeMapping const & l, AttributeMapping const & r)
	{ return l.location == r.location and l.glsl_type == r.glsl_type and l.per_patch == r.per_patch and l.key == r.key; }

	inline bool SameMappingType(AttributeMapping const & l, AttributeMapping const & r)
	{ return l.glsl_type == r.glsl_type and l.per_patch == r.per_patch and l.key == r.key; }

	struct UniformMapping
	{
		u32 location;
		GLenum glsl_type; // just for debug purposes
		std::string key;
	};

	inline bool operator==(UniformMapping const & l, UniformMapping const & r)
	{ return l.location == r.location and l.glsl_type == r.glsl_type and l.key == r.key; }

	inline bool SameMappingType(UniformMapping const & l, UniformMapping const & r)
	{ return l.glsl_type == r.glsl_type and l.key == r.key; }

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

	inline bool operator==(UniformBlockMapping::Variable const & l, UniformBlockMapping::Variable const & r)
	{ return l.offset == r.offset and l.glsl_type == r.glsl_type and l.key == r.key; }

	inline bool operator==(UniformBlockMapping const & l, UniformBlockMapping const & r)
	{ return l.location == r.location and l.data_size == r.data_size and l.key == r.key and l.variables == r.variables; }

	inline bool SameMappingType(UniformBlockMapping::Variable const & l, UniformBlockMapping::Variable const & r)
	{ return l.glsl_type == r.glsl_type and l.key == r.key; }

	inline bool SameMappingType(UniformBlockMapping const & l, UniformBlockMapping const & r)
	{ return l.data_size == r.data_size and l.key == r.key and l.variables == r.variables; }

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

	inline bool operator==(StorageBlockMapping::Variable const & l, StorageBlockMapping::Variable const & r)
	{ return l.offset == r.offset and l.glsl_type == r.glsl_type and l.key == r.key; }

	inline bool operator==(StorageBlockMapping const & l, StorageBlockMapping const & r)
	{ return l.location == r.location and l.data_size == r.data_size and l.key == r.key and l.variables == r.variables; }

	inline bool SameMappingType(StorageBlockMapping::Variable const & l, StorageBlockMapping::Variable const & r)
	{ return l.glsl_type == r.glsl_type and l.key == r.key; }

	inline bool SameMappingType(StorageBlockMapping const & l, StorageBlockMapping const & r)
	{ return l.data_size == r.data_size and l.key == r.key and l.variables == r.variables; }

	// results should be cached for better performance
	template<std::ranges::range Range, typename Key>
	auto & GetMapping(Range const & mappings, Key const & key)
	{
		for (auto & mapping: mappings)
			if (key == mapping.key)
				return mapping;

		assert(("key not found", false));
	}

	// results should be cached for better performance
	template<std::ranges::range Range, typename Key>
	GLint GetLocation(Range const & mappings, Key const & key)
	{
		for (auto & mapping: mappings)
			if (key == mapping.key)
				return mapping.location;

		return -1; // see the Notes section paragraph 4 of https://docs.gl/gl4/glUniform
	}
}