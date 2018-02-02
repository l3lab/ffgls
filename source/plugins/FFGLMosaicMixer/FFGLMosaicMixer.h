#pragma once

#include <FFGLShader.h>
#include "FFGLPluginSDK.h"
#include "FFGLParameter.h"
#include "FFGLFBO.h"
#include <string>
#include <map>
#include <memory>
#include <vector>

#include "GLCommons.h"
#include "GLQuadNode.h"


const GLTexcoords shiftDirections[4] = {
	{ 1.0f, 0.0f },
	{ 0.0f,  -1.0f },
	{ -1.0f, 0.0f },
	{ 0.0f,  1.0f }
};

const GLTexcoords directions[4] = {
	{ -1.0f, 1.0f },
	{ 1.0f,  -1.0f },
	{ 1.0f, 1.0f },
	{ -1.0f,  -1.0f }
};

class FFGLMosaicMixer: public CFreeFrameGLPlugin
{



public:
	FFGLMosaicMixer();
	virtual ~FFGLMosaicMixer();

	///////////////////////////////////////////////////
	// FreeFrame plugin methods
	///////////////////////////////////////////////////

	FFResult SetFloatParameter(unsigned int dwIndex, float value) override;
	float GetFloatParameter(unsigned int index) override;

	FFResult ProcessOpenGL(ProcessOpenGLStruct* pGL) override;
	FFResult InitGL(const FFGLViewportStruct *vp);
	FFResult DeInitGL();
   
	///////////////////////////////////////////////////
	// Factory method
	///////////////////////////////////////////////////
	static FFResult __stdcall CreateInstance(CFreeFrameGLPlugin **ppOutInstance)
	{
		*ppOutInstance = new FFGLMosaicMixer();
		if (*ppOutInstance != NULL)
			return FF_SUCCESS;
		return FF_FAIL;
	}

private:

	FFGLShader m_shader;
	
	bool m_isBillboardQuadConstructed{ false };
	bool m_areRandomQuadsConstructed{ false };

	GLuint m_vaoBillboardQuad{ 0 };
	GLuint m_vaoBillboardRandomQuads{ 0 };
	GLuint m_vboBillboardRandomQuads[3];

	bool m_updateGeometryFlag{ true };
	float m_mixerValue;
	
	

	int m_quadCount{ 0 };
	int m_frameCounter{ 0 };
	int m_dirSwitchCounter{ 0 };

	GLfloat m_shift{ 0 };
	GLTexcoords m_frameShift{ 0.0, 0.0 };
	GLTexcoords m_shiftDirection{ shiftDirections[0].s, shiftDirections[0].t };
	
	void BuildGeometry(int quadCount);

	std::map<std::string, int> attributeValues
	{
		{ "a_vertexCoords", 0 },
		{ "a_textureCoords", 1 },
		{ "a_vertexDirections", 2 }
	};

	
	
};

