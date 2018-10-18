#pragma once

#include <FFGLShader.h>
#include "FFGLPluginSDK.h"
#include "FFGLParameter.h"
#include "FFGLFBO.h"
#include <string>
#include <map>
#include <memory>
#include <vector>



class FFGLVolumeRendering : public CFreeFrameGLPlugin
{
public:

    FFGLVolumeRendering();
    ~FFGLVolumeRendering();

    ///////////////////////////////////////////////////
    // FreeFrame plugin methods
    ///////////////////////////////////////////////////
	
	FFResult SetFloatParameter(unsigned int dwIndex, float value) override;
	float GetFloatParameter(unsigned int index) override;

	FFResult	ProcessOpenGL(ProcessOpenGLStruct* pGL) override;
	FFResult	InitGL(const FFGLViewportStruct *vp) override;
	FFResult	DeInitGL() override;

    ///////////////////////////////////////////////////
    // Factory method
    ///////////////////////////////////////////////////

    static FFResult __stdcall CreateInstance(CFreeFrameGLPlugin **ppInstance)
    {
        *ppInstance = new FFGLVolumeRendering();
        if (*ppInstance != NULL) return FF_SUCCESS;
        return FF_FAIL;
    }


private:

	std::map<int, FFGLParameter> parameterDefinitions;
	std::map<std::string, int> attributeValues
	{
		{ "a_vertexCoords", 0 },
		{ "a_textureCoords", 1 },
		{ "a_vertexDirections", 2 }
	};

	FFGLShader m_shader;

	void BuildGeometry(int quadCount, float gapSize);
	bool m_isGeometryConstructed{ false };
	bool m_updateGeometryFlag{ true };

	
	GLuint m_vaoGeometryQuads{ 0 };
	GLuint m_vboGeometryQuads[2];
	int m_quadCount{ 0 };


#pragma region Parameters
	
	// Scene scale
	float fScaleValue{ 1.0f };
	// Planes angles along X and Y axis
	float fXAngle{ 0.5f };
    float fYAngle{ 0.5f };
	// Planes gap
	float fDistance{ 1.0f };
	// 
	float fPlanesCount{ 0.5f };

#pragma endregion

	
	
	float fIsPerspective;

	float fAngle;

	GLuint iPlanesCount;

	
	
	GLuint iVertexArrayID;
	GLuint iTexcoordArrayID;
	
	
	struct GLVertexTriplet	{	GLfloat x,y,z;	};
	struct GLTexcoords		{	GLfloat s,t;	};

	FFGLVolumeRendering::GLVertexTriplet *VertexData;
	FFGLVolumeRendering::GLTexcoords *TexcoordData;
	
	GLuint iList;	

	GLuint vboId = 0;                   // ID of VBO for vertex arrays

	GLuint vboId1 = 0;
	
	
	bool isGeometryRebuildNeeded;



};




