#include "GLQuadNode.h"


GLQuadNode::GLQuadNode()
{
	Children = NULL;
}


GLQuadNode::~GLQuadNode()
{
	if (this->Children != NULL)
		delete[] this->Children;
}
