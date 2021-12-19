#pragma once

#include ".pch.hpp"

namespace GLFW
{
	struct Context
	{
		static void error_callback(i32 error, const char* description)
		{
			std::cerr << "GLFW Error " << error << ": " << description << '\n';
		}

		Context() = default;

		void create()
		{
			glfwSetErrorCallback(error_callback);

			if (glfwInit())
				std::clog << "GLFW initialized" << '\n';
			else
				throw std::runtime_error("GLFW failed to initialize");
		}

		~Context()
		{
			glfwTerminate();
		}
	};
	
	struct Window
	{
		GLFWwindow* glfw_window = nullptr;

		Window() = default;

		operator GLFWwindow*() const
		{
			return glfw_window;
		}

		static void resize_callback(GLFWwindow*, int width, int height)
		{
			GL::glViewport(0, 0, width, height);
		}

		struct Description
		{
			std::string_view title;
			i32x2 size = {720, 720};
			bool vsync = false;

			i32 const & gl_major;
			i32 const & gl_minor;
		};

		void create(Description const & description)
		{
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, description.gl_major);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, description.gl_minor);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
			if constexpr (DEBUG)
				glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

			glfw_window = glfwCreateWindow(
				description.size.x, description.size.y,
				description.title.data(), nullptr, nullptr
			);

			if (glfw_window != nullptr)
				std::clog << "GLFW Window created" << '\n';
			else
				throw std::runtime_error("GLFW Window failed to create");

			glfwMakeContextCurrent(glfw_window);

			glfwSwapInterval(description.vsync ? 1 : 0);

			// Load OpenGL API
			glbinding::initialize(glfwGetProcAddress);

			// Set callbacks
			if constexpr (DEBUG)
			{
				using namespace GL;

				i32 flags = 0;
				glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
				// TODO(bekorn): when run inside RenderDoc, can not create a debug context
				assert(flags & static_cast<i32>(GL_CONTEXT_FLAG_DEBUG_BIT));

				glEnable(GL_DEBUG_OUTPUT);
				glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
				glDebugMessageCallback(debug_callback, nullptr);
				glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
			}

			glfwSetWindowSizeCallback(glfw_window, resize_callback);
		}

		~Window()
		{
			glfwDestroyWindow(glfw_window);
		}
	};
}
