#pragma once

#include <unordered_map>
#include <iomanip>

#include "core.hpp"

// inspired by https://github.com/skypjack/entt/blob/master/src/entt/core/hashed_string.hpp
// TODO(bekorn): make this constexpr (remove string hashing as much as possible)
// TODO(bekorn): Name.string should only be present when Editing, Game should not care about human readability
struct Name
{
	usize hash;
	std::string string;

	CTOR(Name, default)
	COPY(Name, default)
	MOVE(Name, default)

	Name(std::string_view const & sv) :
		hash(std::hash<std::string_view>{}(sv)), string(sv)
	{}

	Name(std::string const & str) :
		hash(std::hash<std::string>{}(str)), string(str)
	{}

	bool operator==(Name const & other) const
	{ return hash == other.hash; }

	struct Hasher
	{
		usize operator()(Name const & name) const
		{ return name.hash; }
	};

	friend std::ostream & operator<<(std::ostream & out, Name const & name)
	{ return out << std::right << std::setw(20) << name.hash << std::left << '|' << name.string; }
};

Name operator""_name (const char * literal, usize size)
{
	return std::string_view(literal, size);
}

template<typename T>
struct Named
{
	Name const & name;
	T & data;

	bool operator==(Named const & other) const
	{ return name == other.name; }
};

template<typename T>
struct Managed
{
	std::unordered_map<Name, T, Name::Hasher> resources;

	template<typename... Args>
	Named<T> generate(Name const & name, Args && ... args)
	{
		auto [it, is_emplaced] = resources.try_emplace(name, std::forward<Args>(args)...);
		if (not is_emplaced)
			std::cerr << "!! Resource is not emplaced: " << name << '\n';
		return {it->first, it->second};
	}

	void erase(Name const & name)
	{ resources.erase(name); }

	T & get(Name const & name)
	{ return resources.at(name); }

	T const & get(Name const & name) const
	{ return resources.at(name); }

	Named<T> get_named(Name const & name)
	{
		auto it = resources.find(name);
		return { it->first, it->second };
	}

	// TODO(bekorn): there could be a better way of this, returned value does not tell anything valuable,
	//  the caller has to do the same branching on the call site, maybe just copy this code to all the call sites
	//  having begin, end, and find can shorten the call stie code as this one
	T * find(Name const & name)
	{
		auto it = resources.find(name);
		if (it == resources.end())
			return nullptr;
		else
			return &it->second;
	}
};