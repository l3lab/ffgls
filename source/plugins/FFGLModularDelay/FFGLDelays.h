#pragma once

#include <FFGLShader.h>
#include "FFGLPluginSDK.h"
#include "FFGLParameter.h"
#include "FFGLFBO.h"
#include <string>
#include <map>
#include <memory>
#include <vector>


enum class ParamCodes : int
{
	NUM_MODULES,
	PATTERN_MODE,
	CONFIG_BUFFERS_NUMBER
};


enum Pattern : int
{
	VERTICAL_STRIPS = 0,
	HORIZONTAL_STRIPS,
	QUAD_NET_FROM_LLC,
	QUAD_NET_FROM_URC,
	LAST_ONE
};



static void HStrip(double lPos, double width, double lTexPos, double texWidth);
static void VStrip(double lPos, double width, double lTexPos, double texWidth);
static FFGLTextureStruct GetMultipliers(FFGLTextureStruct textureDesc, int modulesCount);


class FFGLDelays : public CFreeFrameGLPlugin
{
private:

	
	std::map<int, FFGLParameter> parameterDefinitions;
	std::vector<FFGLFBO> fbos;

	int bufferCount{ 121 };
	int index{ 0 };

	std::string bufferNumber{ "120" };
	GLfloat modulesAmountCoeff{ 1.0f };
	GLfloat patternCoeff{ 1.0f };

	FFGLViewportStruct viewPoint;

	inline int GetOldest() { return (index + 1) % bufferCount; }
	inline int GetNewest() { return index; }

	inline int GetIndex() { return GetNewest(); }
	inline int IncIndex() { index = GetOldest(); return index; }

	Pattern GetQuadType()
	{
		auto ptrn = (Pattern)((int)(patternCoeff * (float)Pattern::LAST_ONE));
		return (ptrn == Pattern::LAST_ONE) ? QUAD_NET_FROM_URC: ptrn;
	}

	void processParamsValuesChanges();
	void reConstructBuffers(const FFGLViewportStruct vp, int bufferCount);


public:

	FFGLDelays();
	~FFGLDelays();

	FFResult SetFloatParameter(unsigned int dwIndex, float value) override;
	float GetFloatParameter(unsigned int index) override;

	FFResult SetTextParameter(unsigned int index, const char *value) override;
	char* GetTextParameter(unsigned int index) override;

	FFResult	ProcessOpenGL(ProcessOpenGLStruct* pGL) override;
	FFResult	InitGL(const FFGLViewportStruct *vp) override;
	FFResult	DeInitGL() override;


	///////////////////////////////////////////////////
	// Factory method
	///////////////////////////////////////////////////
	static FFResult __stdcall CreateInstance(CFreeFrameGLPlugin **ppOutInstance)
	{
		*ppOutInstance = new FFGLDelays();
		if (*ppOutInstance != NULL)
			return FF_SUCCESS;
		return FF_FAIL;
	}
};
