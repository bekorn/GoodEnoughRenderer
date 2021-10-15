#pragma once

#include ".pch.hpp"
#include "core_types.hpp"

namespace GL
{
	struct FrameBuffer : OpenGLObject
	{
		FrameBuffer() noexcept = default;
		FrameBuffer(FrameBuffer &&) noexcept = default;

		~FrameBuffer()
		{
			glDeleteFramebuffers(1, &id);
		}

		struct Description
		{
			struct Attachment
			{
				GLenum type;
				Texture2D const & texture;
			};

			std::vector<Attachment> const & attachments;
		};

		void create(Description const & description)
		{
			glGenFramebuffers(1, &id);

			glBindFramebuffer(GL_FRAMEBUFFER, id);

			for (auto const & attachment: description.attachments)
			{
				glFramebufferTexture2D(
					GL_FRAMEBUFFER, attachment.type,
					GL_TEXTURE_2D, attachment.texture.id, 0
				);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	};
}
