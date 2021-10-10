#pragma once

#include ".pch.hpp"

struct GLFWContext
{
	static void error_callback(i32 error, const char* description)
	{
		std::cerr << "GLFW Error " << error << ": " << description << '\n';
	}

	GLFWContext() = default;

	void create()
	{
		glfwSetErrorCallback(error_callback);

		if (glfwInit())
			std::clog << "GLFW initialized" << '\n';
		else
			throw std::runtime_error("GLFW failed to initialize");
	}

	~GLFWContext()
	{
		glfwTerminate();
	}
};