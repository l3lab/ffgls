#pragma once
#include "FFGL.h"
#include "FFGLLib.h"

struct FrameBuffer
{
	GLuint bufferId;
	GLuint textureId;
	GLenum status;
};

class FrameBufferUtils
{
	public:	
		static FrameBuffer CreateFrameBuffer(GLsizei width, GLsizei height);
		static void DeleteFrameBuffer(FrameBuffer& fb);
		static void FrameRect(FFGLTexCoords& maxCoords);
};

