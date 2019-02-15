#pragma once

#include <FFGLShader.h>
#include "FFGLPluginSDK.h"
#include "FFGLParameter.h"
#include "FFGLFBO.h"
#include <string>
#include <map>
#include <memory>
#include <vector>

#include "FFGLShaderCodes.h"

#define mulFtoI(v1, v2) (int)(((float)(v1))*((float)(v2)))

static std::vector<GLuint> texIds;
static int instanceCounter = 0;

/* 
	Parameters codes
*/
enum class ParamNames : int
{
	FUNC_DEF,
	NOISE_FACTOR,
	IMG_BLENDING_FACTOR,
	NOISES_TEXTURES_COUNT,
	NOISES_SCALE,
	OPERATOR_TYPE,	
	X1,
	Y1,
	X2,
	Y2,
	X3,
	Y3,
	VELOCITY,
	VELOCITY_SCALE
	
};

struct ParamDefinition
{
public:
	std::string paramName;
	DWORD		paramType;
	float*		floatValueStorage;		
};

class FFGLFlows : public CFreeFrameGLPlugin
{
public:

	FFGLFlows();
	virtual ~FFGLFlows();

	///////////////////////////////////////////////////
	// FreeFrame plugin methods
	///////////////////////////////////////////////////

	FFResult SetFloatParameter(unsigned int dwIndex, float value) override;
	float GetFloatParameter(unsigned int index) override;

	FFResult SetTextParameter(unsigned int index, const char *value) override;
	char* GetTextParameter(unsigned int index) override;

	//DWORD	SetParameter(const SetParameterStruct* pParam);
	//DWORD	GetParameter(DWORD dwIndex);

	FFResult	ProcessOpenGL(ProcessOpenGLStruct* pGL) override;
	FFResult	InitGL(const FFGLViewportStruct *vp) override;
	FFResult	DeInitGL() override;


	
	///////////////////////////////////////////////////
	// Factory method
	///////////////////////////////////////////////////
	static FFResult __stdcall CreateInstance(CFreeFrameGLPlugin **ppOutInstance)
	{
		*ppOutInstance = new FFGLFlows();
		if (*ppOutInstance != NULL)
			return FF_SUCCESS;
		return FF_FAIL;
	}

protected:



private:

	class FBOPair
	{
	public:
		GLuint firstBufferId;
		GLuint firstTextureId;

		GLuint secondBufferId;
		GLuint secondTextureId;

		bool reversedDirection;

	} *fbos{ nullptr };

	std::map<int, FFGLParameter> parameterDefinitions;
	std::map<std::string, int> attributeValues
	{
		{ "a_vertexCoords", 0 },
		{ "a_textureCoords", 1 },
		{ "a_vertexDirections", 2 }
	};

	FFGLShader analyticShader;
	FFGLShader sobelShader;
	FFGLShader directShader;

	FFGLShader defaultShader;

	GLfloat alphaNoisesTexture{ 0.5f };
	GLfloat alphaImageTexture{ 0.5f };

	float x1{ 0.0f }, x2{ 0.0f }, x3{ 0.0f };
	float y1{ 0.0f }, y2{ 0.0f }, y3{ 0.0f };
	
	float velocity{ 0.5f };
	float velocityScale{ 0.5f };
	float noiseDimScale{ 0.5f };
	float noiseTexturesCountFactor{ 0.5f };
	float operatorTypeFactor{ 0.5 };
	
	


	GLuint noiseWidth;	
	GLuint noiseHeight;
	
	int currentTexId{ 0 };
	bool IsFirstFrame{ true };
	bool shaderCodeChanged{ false };
	string fieldCode;

	GLuint frameBufferId;
	GLuint renderedTexture;


	void DeleteNoiseTextures();	
	void EnsureTexturesCreated(int width, int height);
	FBOPair* CreateFBO(const GLuint width, const GLuint height);




};