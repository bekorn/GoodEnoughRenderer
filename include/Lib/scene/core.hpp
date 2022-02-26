#pragma once

#include <queue>

#include "Lib/core/core.hpp"
#include "Lib/render/mesh.hpp"

namespace Scene
{
	struct Transform
	{
		f32x3 position{0, 0, 0};
		f32quat rotation{1, 0, 0, 0};
		f32x3 scale{1, 1, 1};

		f32x4x4 calculate_transform() const
		{
			f32x4x4 transform(1);
			transform *= glm::translate(position);
			transform *= glm::mat4_cast(rotation);
			transform *= glm::scale(scale);
			return transform;
		}
	};

	struct Tree
	{
		struct Node
		{
			Name name;
			u32 depth;
			u32 parent_index;
			Transform transform;
			f32x4x4 matrix;
			Render::Mesh * mesh = nullptr;
		};
		vector<vector<Node>> nodes{1}; // root depth always exists

		struct Index
		{
			u32 depth;
			u32 index;
		};
		Managed<Index> named_indices;

		u32 version = 0;

		Index add(Node const & node)
		{
			version++;

			if (nodes.size() < node.depth + 1)
				nodes.resize(node.depth + 1);

			auto index = Index{.depth = node.depth, .index = u32(nodes[node.depth].size())};

			nodes[index.depth].push_back(node);
			named_indices.generate(node.name, index);

			return index;
		}

		Node & get(Index const & index)
		{ return nodes[index.depth][index.index]; }

		usize size() const
		{
			usize size = 0;
			for (auto & depth : nodes)
				size += depth.size();
			return size;
		}

		void update_transforms()
		{
			for (auto & node: nodes[0])
				node.matrix = node.transform.calculate_transform();

			for (auto depth = 1; depth < nodes.size(); ++depth)
				for (auto & node: nodes[depth])
					node.matrix = nodes[depth - 1][node.parent_index].matrix * node.transform.calculate_transform();
		}

		struct DepthFirst
		{
			u32 version;
			std::unique_ptr<Node *[]> traversal;
			Node ** traversal_end;

			explicit DepthFirst(Tree & tree):
				version(tree.version),
				traversal(new Node*[tree.size()]),
				traversal_end(traversal.get() + tree.size())
			{
				std::forward_list<Node *> linked_traversal;
				std::unordered_map<const Node *, std::forward_list<Node *>::const_iterator> node2iter;

				for (auto & node: tree.nodes[0])
				{
					linked_traversal.push_front(&node);
					node2iter.emplace(&node, linked_traversal.begin());
				}

				for (auto depth = 1; depth < tree.nodes.size(); ++depth)
					for (auto & node: tree.nodes[depth])
					{
						auto const & parent_node = tree.nodes[depth - 1][node.parent_index];
						auto const & parent_iter = node2iter.at(&parent_node);
						auto node_iter = linked_traversal.insert_after(parent_iter, &node);
						node2iter.emplace(&node, node_iter);
					}

				std::copy(linked_traversal.begin(), linked_traversal.end(), traversal.get());
			}

			struct Iterator
			{
				Node ** current;

				void operator++()
				{ current++; }

				Node & operator*()
				{ return **current; }

				bool operator!=(Iterator const & other)
				{ return current != other.current; }
			};

			Iterator begin() const
			{ return {traversal.get()}; }

			Iterator end() const
			{ return {traversal_end}; }
		};

		unique_ptr<DepthFirst> _cached_depth_first;
		DepthFirst const & depth_first()
		{
			if (_cached_depth_first == nullptr || _cached_depth_first->version != version)
				_cached_depth_first = std::make_unique<DepthFirst>(*this);

			return *_cached_depth_first;
		}
	};
}

