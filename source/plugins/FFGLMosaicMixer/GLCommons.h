#pragma once

#include "FFGLPluginSDK.h"

struct GLVertex	
{ 
	GLfloat x, y, z;
	GLVertex(GLfloat px, GLfloat py, GLfloat pz)
	{
		x = px;
		y = py;
		z = pz;
	}

	GLVertex()
	{
		GLVertex(0, 0, 0);
	}

};
struct GLTexcoords		{ GLfloat s, t; };

struct GLQuad 
{
	GLVertex verts[4]; 
	GLTexcoords texcoords[4]; 

	GLQuad(){};

	GLQuad(GLVertex lowerLeft, GLVertex lowerRight, GLVertex upperRight, GLVertex upperLeft)
	{
		verts[0] = lowerLeft;
		verts[1] = lowerRight;
		verts[2] = upperRight;
		verts[3] = upperLeft;
	}
};

#define STRINGIFY( expr ) std::string(#expr)
