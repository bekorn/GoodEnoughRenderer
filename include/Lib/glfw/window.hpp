#pragma once

#include ".pch.hpp"
#include "Lib/opengl/.pch.hpp"

namespace GLFW
{
	using namespace gl43core;

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
			glViewport(0, 0, width, height);
		}

		static void debug_callback(
			GLenum source,
			GLenum type, GLuint id, GLenum severity,
			GLsizei length, const GLchar* message,
			const void* userParam
		)
		{
			using namespace gl43core;

			auto & out = std::clog;

			// ignore non-significant error/warning codes
			if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
				return;

			out << "OpenGL Debug message (" << id << "): " << message << '\n';

			out << "[";
			switch (source)
			{
			case GL_DEBUG_SOURCE_API:
				out << "Source: API";
				break;
			case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
				out << "Source: Window System";
				break;
			case GL_DEBUG_SOURCE_SHADER_COMPILER:
				out << "Source: Shader Compiler";
				break;
			case GL_DEBUG_SOURCE_THIRD_PARTY:
				out << "Source: Third Party";
				break;
			case GL_DEBUG_SOURCE_APPLICATION:
				out << "Source: Application";
				break;
			case GL_DEBUG_SOURCE_OTHER:
				out << "Source: Other";
				break;
			default:
				break;
			}
			out << ", ";

			switch (type)
			{
			case GL_DEBUG_TYPE_ERROR:
				out << "Type: Error";
				break;
			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
				out << "Type: Deprecated Behaviour";
				break;
			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
				out << "Type: Undefined Behaviour";
				break;
			case GL_DEBUG_TYPE_PORTABILITY:
				out << "Type: Portability";
				break;
			case GL_DEBUG_TYPE_PERFORMANCE:
				out << "Type: Performance";
				break;
			case GL_DEBUG_TYPE_MARKER:
				out << "Type: Marker";
				break;
			case GL_DEBUG_TYPE_PUSH_GROUP:
				out << "Type: Push Group";
				break;
			case GL_DEBUG_TYPE_POP_GROUP:
				out << "Type: Pop Group";
				break;
			case GL_DEBUG_TYPE_OTHER:
				out << "Type: Other";
				break;
			default:
				break;
			}
			out << ", ";

			switch (severity)
			{
			case GL_DEBUG_SEVERITY_HIGH:
				out << "Severity: high";
				break;
			case GL_DEBUG_SEVERITY_MEDIUM:
				out << "Severity: medium";
				break;
			case GL_DEBUG_SEVERITY_LOW:
				out << "Severity: low";
				break;
			case GL_DEBUG_SEVERITY_NOTIFICATION:
				out << "Severity: notification";
				break;
			default:
				break;
			}
			out << "]" << '\n';
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