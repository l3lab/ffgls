#include "FrameBufferUtils.h"

FrameBuffer FrameBufferUtils::CreateFrameBuffer(GLsizei width, GLsizei height)
{
	FrameBuffer fb{ 0, 0, GL_FRAMEBUFFER_COMPLETE_EXT };

	glGenFramebuffers(1, &fb.bufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, fb.bufferId);

	glGenTextures(1, &fb.textureId);
	glBindTexture(GL_TEXTURE_2D, fb.textureId);	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb.textureId, 0);

	fb.status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER);

	return fb;
}

void FrameBufferUtils::DeleteFrameBuffer(FrameBuffer& fb)
{
	if (fb.textureId != 0)	
		glDeleteTextures(1, &fb.textureId);

	if (fb.bufferId != 0)
		glDeleteFramebuffers(1, &fb.bufferId);		
}

void FrameBufferUtils::FrameRect(FFGLTexCoords& maxCoords)
{
	glBegin(GL_QUADS);
	
	//lower left
	glTexCoord2d(0.0, 0.0);
	glVertex2f(-1, -1);

	//upper left	
	glTexCoord2d(0.0, maxCoords.t);
	glVertex2f(-1, 1);

	//upper right	
	glTexCoord2d(maxCoords.s, maxCoords.t);
	glVertex2f(1, 1);

	//lower right
	glTexCoord2d(maxCoords.s, 0.0);
	glVertex2f(1, -1);

	glEnd();
}

