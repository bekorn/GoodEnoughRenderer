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
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/range.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/string_cast.hpp>

// TODO(bekorn): fmtlib is not enough to remove std::*stream completely because it does not support parsing,
//  there is an fmt-like lib: https://github.com/eliaskosunen/scnlib but, benchmarks show increase in compile times
// fmt
#include <fmt/core.h>
#include <fmt/std.h>

// Handy std classes
#include <memory>
#include <span>
#include <optional>
#include <variant>

// std containers
#include <vector>
#include <array>
#include <unordered_map>

// std utilities
#include <sstream> // TODO(bekorn): replace with a better lib
#include <chrono>
#include <source_location>
