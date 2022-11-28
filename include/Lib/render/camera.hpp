#pragma once

#include "Lib/core/core.hpp"

template<typename C>
concept Camera =
requires(C const camera)
{
	std::same_as<decltype(camera.position), f32x3>;
	std::same_as<decltype(camera.up), f32x3>;
	std::same_as<decltype(camera.target), f32x3>;
	{ camera.get_view() } -> std::same_as<f32x4x4>;
	{ camera.get_projection() } -> std::same_as<f32x4x4>;
	{ camera.get_view_without_translate() } -> std::same_as<f32x4x4>;
};

struct PerspectiveCamera //final : ICamera
{
	f32x3 position;
	f32x3 up;
	f32x3 target;
	f32 fov;
	f32 near, far;
	f32 aspect_ratio;

	f32x4x4 get_view() const
	{ return glm::lookAt(position, target, up); }

	f32x4x4 get_projection() const
	{ return glm::perspective(glm::radians(fov), aspect_ratio, near, far); }

	f32x4x4 get_view_without_translate() const
	{ return glm::lookAt(f32x3(0), target - position, up); }
};

struct OrthographicCamera
{
	f32x3 position;
	f32x3 up;
	f32x3 target;
	f32 left, right;
	f32 bottom, top;

	f32x4x4 get_view() const
	{ return glm::lookAt(position, target, up); }

	f32x4x4 get_projection() const
	{ return glm::ortho(left, right, bottom, top); }

	f32x4x4 get_view_without_translate() const
	{ return glm::lookAt(f32x3(0), target - position, up); }
};