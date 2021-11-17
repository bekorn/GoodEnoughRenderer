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
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/component_wise.hpp>


// Handy std classes
#include <memory>
#include <optional>
#include <span>

