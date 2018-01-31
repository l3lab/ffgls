#pragma once

#include "GLCommons.h"

enum GLQuadElems { LOWER_LEFT = 0, LOWER_RIGHT = 1, UPPER_RIGHT = 2, UPPER_LEFT = 3 };

class GLQuadNode
{
public:

	GLQuad Quad;
	
	GLQuadNode(GLVertex lowerLeft, GLVertex lowerRight, GLVertex upperRight, GLVertex upperLeft)
	{
		this->Quad = GLQuad(lowerLeft, lowerRight, upperRight, upperLeft);
	}

	void SetChild(GLQuadElems i, GLQuadNode* ptrValue)
	{
		if (Children == NULL)
			Children = new GLQuadNode*[4];
		
		Children[i] = ptrValue;
	}


	const GLQuadNode * operator[] (GLQuadElems i)
	{
		return (Children != NULL) ? Children[i] : NULL;			 
	}


	static void SubdivideNode(GLQuadNode *nodeToDivide)
	{
		if (nodeToDivide != NULL)
		{
			float xAlpha = (float)rand() / (float)(RAND_MAX + 1);
			float yAlpha = (float)rand() / (float)(RAND_MAX + 1);
			GLVertex rndVr;

			rndVr.x = xAlpha * nodeToDivide->Quad.verts[0].x + (1 - xAlpha) * nodeToDivide->Quad.verts[1].x;
			rndVr.y = xAlpha * nodeToDivide->Quad.verts[0].y + (1 - xAlpha) * nodeToDivide->Quad.verts[3].y;
			rndVr.z = nodeToDivide->Quad.verts[0].y + 0.001;

			GLVertex tVrt = nodeToDivide->Quad.verts[0];

			nodeToDivide->SetChild(GLQuadElems::LOWER_LEFT, new GLQuadNode(GLVertex(tVrt.x, tVrt.y, rndVr.z), GLVertex(rndVr.x, tVrt.y, rndVr.z), rndVr, GLVertex(tVrt.x, rndVr.y, rndVr.z)));
			/*
			(*nodeToDivide)[GLQuadElems::LOWER_RIGHT] = new GLQuadNode(GLVertex(tVrt.x, tVrt.y, rndVr.z), GLVertex(rndVr.x, tVrt.y, rndVr.z), rndVr, GLVertex(tVrt.x, rndVr.y, rndVr.z));
			(*nodeToDivide)[GLQuadElems::UPPER_RIGHT] = new GLQuadNode(GLVertex(tVrt.x, tVrt.y, rndVr.z), GLVertex(rndVr.x, tVrt.y, rndVr.z), rndVr, GLVertex(tVrt.x, rndVr.y, rndVr.z));
			(*nodeToDivide)[GLQuadElems::UPPER_LEFT] = new GLQuadNode(GLVertex(tVrt.x, tVrt.y, rndVr.z), GLVertex(rndVr.x, tVrt.y, rndVr.z), rndVr, GLVertex(tVrt.x, rndVr.y, rndVr.z));
			*/
		}
	}

	GLQuadNode();
	~GLQuadNode();

private: 
	GLQuadNode ** Children = NULL;


};


