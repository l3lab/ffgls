#pragma once

#include <FFGLShader.h>
#include "FFGLPluginSDK.h"
#include "FFGLParameter.h"
#include "FFGLFBO.h"
#include <string>
#include <map>
#include <memory>
#include <vector>

class FFGLTriangleMixer : public CFreeFrameGLPlugin
{
public:

	enum TexPattern:unsigned char 
	{
		SplitAsGeometry = 0, // All triangles use splitted texcoords
		TwoCentralOnly = 1, // Only two central triangles use central texmapping
		WithAdjacents = 2, // Two central triangles and four adjacent triangles use central texmapping
		All = 3 // All triangles use central texmapping
	};

	FFGLTriangleMixer(void);
	~FFGLTriangleMixer(void);


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
		*ppOutInstance = new FFGLTriangleMixer();
		if (*ppOutInstance != NULL)
			return FF_SUCCESS;
		return FF_FAIL;
	}

private:
	// Parameters
	float m_blend;
	TexPattern pattern;
	FFGLShader m_shader;
		
	DWORD ScalableTriangleMeshMix(ProcessOpenGLStruct* pGL);

};
