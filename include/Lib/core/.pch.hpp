#pragma once

// Configure GLM
#define GLM_FORCE_NO_CTOR_INIT
#define GLM_FORCE_EXPLICIT_CTOR

// forward declarations to reduce compile times
#include <glm/fwd.hpp>

// source file
#include <glm/glm.hpp>

// extensions (http://glm.g-truc.net/glm.pdf)
#include <glm/gtc/type_precision.hpp>
#include <glm/gtx/range.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>


// Handy std classes
#include <memory>
#include <vector>
#include <array>
#include <span>
#include <optional>
#include <variant>

