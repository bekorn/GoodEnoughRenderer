#pragma once

#include "core.hpp"
#include "texture_2d.hpp"

namespace GL
{
	struct FrameBuffer : OpenGLObject
	{
		i32x2 resolution;
		Texture2D color0, color1, color2, color3;
		Texture2D depth, stencil;

		CTOR(FrameBuffer, default)
		COPY(FrameBuffer, delete)
		MOVE(FrameBuffer, default)

		~FrameBuffer()
		{
			glDeleteFramebuffers(1, &id);
		}

		struct Description
		{
			i32x2 resolution;

			struct Attachment
			{
				Texture2D FrameBuffer::* type;
				Texture2D::AttachmentDescription const & description;
			};
			vector<Attachment> const & attachments;
		};

		void create(Description const & description)
		{
			resolution = description.resolution;

			for (auto & [attachment_type, attachment_description]: description.attachments)
			{
				auto desc = attachment_description;
				desc.dimensions = resolution;

				Texture2D & attachment = *this.*attachment_type;
				attachment.create(desc);
			}

			glCreateFramebuffers(1, &id);
			glNamedFramebufferTexture(id, GL_DEPTH_ATTACHMENT, depth.id, 0);
			glNamedFramebufferTexture(id, GL_STENCIL_ATTACHMENT, stencil.id, 0);
			glNamedFramebufferTexture(id, GL_COLOR_ATTACHMENT0, color0.id, 0);
			glNamedFramebufferTexture(id, GL_COLOR_ATTACHMENT1, color1.id, 0);
			glNamedFramebufferTexture(id, GL_COLOR_ATTACHMENT2, color2.id, 0);
			glNamedFramebufferTexture(id, GL_COLOR_ATTACHMENT3, color3.id, 0);
		}
	};
}
