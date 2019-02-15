#include <FFGL.h>
#include <FFGLLib.h>
#include <gl\GLU.h>

#include "FFGLFlows.h"

#include "../../lib/ffgl/utilities/utilities.h"

using namespace std;

const int NTEX_COUNT = 256;
const int STEPS_COUNT{ 9 };
const int steps[STEPS_COUNT]{ 256, 128, 64, 32, 16, 8, 4, 2, 1 };


static CFFGLPluginInfo PluginInfo(
	FFGLFlows::CreateInstance,	// Create method
	"FLOW",						// Plugin unique ID
	"FFGLFlows",				// Plugin name											
	1,						   	// API major version number 											
	500,						// API minor version number	
	1,							// Plugin major version number
	000,						// Plugin minor version number
	FF_EFFECT,					// Plugin type
	"FFGL Flows Visualisation",	// Plugin description
	"by Oleg Potiy"				// About
);


static std::string ShaderCodeConcat(std::string* fscUniform, std::string* fscFuncCode, std::string* fieldCode, std::string* fscAdvectionFunc)
{
	std::string analyticShaderCode{ *fscUniform + *fscFuncCode + ((fieldCode == nullptr) ? "}" : *fieldCode) + "}" + *fscAdvectionFunc };
	return analyticShaderCode;
};

static const std::map<int, FFGLParameter> lDefs
{
	{(int)ParamNames::FUNC_DEF, FFGLParameter{ "Field def", defaultCalulations }},

	{(int)ParamNames::NOISE_FACTOR, FFGLParameter{ "Noise factor", 0.5f}},
	{(int)ParamNames::NOISES_TEXTURES_COUNT, FFGLParameter{ "Noises count", 0.5f}},
	{(int)ParamNames::NOISES_SCALE, FFGLParameter{ "Noises scale", 0.5f}},
	{(int)ParamNames::IMG_BLENDING_FACTOR, FFGLParameter{ "Img fraction", 0.5f } },
	{(int)ParamNames::OPERATOR_TYPE, FFGLParameter{ "Operator type", 0.5f} },

	{(int)ParamNames::X1, FFGLParameter{ "X1 Param", 0.0f}},
	{(int)ParamNames::Y1, FFGLParameter{ "Y1 Param", 0.0f}},

	{(int)ParamNames::X2, FFGLParameter{ "X2 Param", 0.0f}},
	{(int)ParamNames::Y2, FFGLParameter{ "Y2 Param", 0.0f}},

	{(int)ParamNames::X3, FFGLParameter{ "X3 Param", 0.0f}},
	{(int)ParamNames::Y3, FFGLParameter{ "Y3 Param", 0.0f}},

	{(int)ParamNames::VELOCITY, FFGLParameter{ "Velosity", 0.5f}},
	{(int)ParamNames::VELOCITY_SCALE, FFGLParameter{ "Velosity Scale", 0.5f} }
};


FFGLFlows::FFGLFlows() :CFreeFrameGLPlugin()
{
	// Input properties
	SetMinInputs(1);
	SetMaxInputs(1);
	//
	parameterDefinitions.insert(lDefs.begin(), lDefs.end());
	//
	for (auto param : parameterDefinitions)
	{
		auto value = param.second;
		if (value.getType() == FF_TYPE_STANDARD)
			SetParamInfo(param.first, value.getName().c_str(), value.getType(), value.getFloatStorage());
		else if (value.getType() == FF_TYPE_BOOLEAN)
			SetParamInfo(param.first, value.getName().c_str(), value.getType(), value.getFloatStorage());
		else if (value.getType() == FF_TYPE_TEXT)
			SetParamInfo(param.first, value.getName().c_str(), value.getType(), value.getStrStorage().c_str());
	}

	instanceCounter += 1;
}

FFGLFlows::~FFGLFlows()
{
	instanceCounter -= 1;
	parameterDefinitions.clear();
	DeleteNoiseTextures();
}


FFResult FFGLFlows::InitGL(const FFGLViewportStruct *vp)
{
	this->noiseWidth = vp->width;
	this->noiseHeight = vp->height;
	
	int compilationResult{ 0 };
	
	const string sobelShaderCode{ UNIFORMS + SOBEL_FIELD + ADVECTION };
	compilationResult = sobelShader.Compile(vertexMulticoordShaderCode, sobelShaderCode);
	if (compilationResult == GL_FALSE)
		return FF_FAIL;

	const string imageShaderCode{ UNIFORMS + IMAGE_FIELD + ADVECTION };
	compilationResult = directShader.Compile(vertexMulticoordShaderCode, imageShaderCode);
	if (compilationResult == GL_FALSE)
		return FF_FAIL;

	string analyticShaderCode{ UNIFORMS + getFieldFuncStart + defaultCalulations + getFieldFuncEnd + ADVECTION };
	compilationResult = analyticShader.Compile(vertexMulticoordShaderCode, analyticShaderCode);
	if (compilationResult == GL_FALSE)
		return FF_FAIL;

	compilationResult = defaultShader.Compile(vertexMulticoordShaderCode, defaultFragmentShaderCode);
	if (compilationResult == GL_FALSE)
		return FF_FAIL;

	defaultShader.BindShader();
	glUniform1i(defaultShader.FindUniform("inputTexture"), 0);
	defaultShader.UnbindShader();
	

	

	return FF_SUCCESS;
}



FFResult FFGLFlows::DeInitGL()
{

	if (analyticShader.IsReady())
		analyticShader.FreeGLResources();

	if (sobelShader.IsReady())
		sobelShader.FreeGLResources();

	if (directShader.IsReady())
		directShader.FreeGLResources();

	if (defaultShader.IsReady())
		defaultShader.FreeGLResources();

	return FF_SUCCESS;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////

FFResult FFGLFlows::ProcessOpenGL(ProcessOpenGLStruct *pGL)
{

	if (pGL->numInputTextures < 1)
		return FF_FAIL;

	if (pGL->inputTextures[0] == NULL)
		return FF_FAIL;

	FFGLTextureStruct Texture{ *(pGL->inputTextures[0]) };

	GLint systemCurrentFbo;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &systemCurrentFbo);

	if (texIds.empty())
		EnsureTexturesCreated(Texture.Width, Texture.Height);

	if (this->fbos == nullptr)
		this->fbos = CreateFBO(Texture.Width, Texture.Height);
		
	//get the max s,t that correspond to the 
	//width,height of the used portion of the allocated texture space
	FFGLTexCoords maxCoords{ GetMaxGLTexCoords(Texture) };

	

	GLuint srcTexture;
	GLuint dstRenderBuffer;
	GLuint dstTexture;



	if (fbos->reversedDirection)
	{	
		srcTexture = fbos->secondTextureId;
		dstRenderBuffer = fbos->firstBufferId;
		dstTexture = fbos->firstTextureId;
		fbos->reversedDirection = false;		
	}
	else
	{		
		srcTexture = IsFirstFrame ? texIds[currentTexId] : fbos->firstTextureId;
		dstRenderBuffer = fbos->secondBufferId;
		dstTexture = fbos->secondTextureId;

		fbos->reversedDirection = true;
		IsFirstFrame = false;
	}

	
	glEnable(GL_TEXTURE_2D);	

	/*
	glBindFramebuffer(GL_FRAMEBUFFER, dstRenderBuffer);
	glClear(GL_COLOR_BUFFER_BIT);

	defaultShader.BindShader();
	glBindTexture(GL_TEXTURE_2D, texIds[currentTexId]);

	glBegin(GL_QUADS);

	//lower left	
	glTexCoord2d(0, 0);
	glVertex2f(-1, -1);

	//upper left
	glTexCoord2d(0, maxCoords.t);
	glVertex2f(-1, 1);

	//upper right
	glTexCoord2d(maxCoords.s, maxCoords.t);
	glVertex2f(1, 1);

	//lower right
	glTexCoord2d(maxCoords.s, 0);
	glVertex2f(1, -1);
	glEnd();

	defaultShader.UnbindShader();
	*/


	// set rendering destination to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, dstRenderBuffer);
	glClear(GL_COLOR_BUFFER_BIT);


	FFGLShader* usedShader = this->operatorTypeFactor > 0.5 ? (&directShader) : (&sobelShader);

	if (this->operatorTypeFactor == 0)
	{		
		if (shaderCodeChanged)
		{
			string analyticShaderCode{ UNIFORMS + getFieldFuncStart + fieldCode + getFieldFuncEnd + ADVECTION };
			analyticShader.Compile(vertexMulticoordShaderCode, analyticShaderCode);			
			shaderCodeChanged = false;
		}

		usedShader = &analyticShader;
	}

	usedShader->BindShader();

	glUniform1i(usedShader->FindUniform("texture0"), 0);
	glUniform1i(usedShader->FindUniform("texture1"), 1);
	glUniform1i(usedShader->FindUniform("texture2"), 2);

	glUniform1f(usedShader->FindUniform("alpha"), 0.1*alphaNoisesTexture);
	glUniform1f(usedShader->FindUniform("imgFactor"), alphaImageTexture);
	glUniform1f(usedShader->FindUniform("velocity"), this->velocity);
	glUniform1f(usedShader->FindUniform("velocityScale"), this->velocityScale);
	glUniform1f(usedShader->FindUniform("noiseScale"), this->noiseDimScale);

	glUniform1f(usedShader->FindUniform("x1"), this->x1);
	glUniform1f(usedShader->FindUniform("y1"), this->y1);

	glUniform1f(usedShader->FindUniform("x2"), this->x2);
	glUniform1f(usedShader->FindUniform("y2"), this->y2);

	glUniform1f(usedShader->FindUniform("x3"), this->x3);
	glUniform1f(usedShader->FindUniform("y3"), this->y3);


	glUniform1f(usedShader->FindUniform("dx"), 1.0f / (float)Texture.Width);
	glUniform1f(usedShader->FindUniform("dy"), 1.0f / (float)Texture.Height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, srcTexture);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texIds[currentTexId]);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, Texture.Handle);

	glBegin(GL_QUADS);

	//lower left
	glMultiTexCoord2d(GL_TEXTURE0, 0, 0);
	glMultiTexCoord2d(GL_TEXTURE1, 0, 0);
	glMultiTexCoord2d(GL_TEXTURE2, 0, 0);
	glVertex2f(-1, -1);

	//upper left
	glMultiTexCoord2d(GL_TEXTURE0, 0, 1);//maxCoords.t);
	glMultiTexCoord2d(GL_TEXTURE1, 0, 1);//maxCoords.t);
	glMultiTexCoord2d(GL_TEXTURE2, 0, maxCoords.t);
	glVertex2f(-1, 1);

	//upper right
	glMultiTexCoord2d(GL_TEXTURE0, 1, 1);// maxCoords.s, maxCoords.t);
	glMultiTexCoord2d(GL_TEXTURE1, 1, 1);//maxCoords.s, maxCoords.t);
	glMultiTexCoord2d(GL_TEXTURE2, maxCoords.s, maxCoords.t);
	glVertex2f(1, 1);

	//lower right
	glMultiTexCoord2d(GL_TEXTURE0, 1, 0);// maxCoords.s, 0);
	glMultiTexCoord2d(GL_TEXTURE1, 1, 0);// maxCoords.s, 0);
	glMultiTexCoord2d(GL_TEXTURE2, maxCoords.s, 0);
	glVertex2f(1, -1);
	glEnd();

	//unbind the input texture
	glBindTexture(GL_TEXTURE_2D, 0);
	usedShader->UnbindShader();
	

	///
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, systemCurrentFbo);

	defaultShader.BindShader();

	glBindTexture(GL_TEXTURE_2D, dstTexture);
	//glBindTexture(GL_TEXTURE_2D, texIds[currentTexId]);

	glBegin(GL_QUADS);

	//lower left	
	glTexCoord2d(0, 0);
	glVertex2f(-1, -1);

	//upper left
	glTexCoord2d(0, 1); //maxCoords.t);
	glVertex2f(-1, 1);

	//upper right
	glTexCoord2d(1, 1);// (maxCoords.s, maxCoords.t);
	glVertex2f(1, 1);

	//lower right
	glTexCoord2d(1, 0); (maxCoords.s, 0);
	glVertex2f(1, -1);
	glEnd();

	defaultShader.UnbindShader();

	glMatrixMode(GL_PROJECTION);

	//Restoring projection matrix
	glPopMatrix();

	//unbind the texture
	glBindTexture(GL_TEXTURE_2D, 0);

	//disable texturemapping
	glDisable(GL_TEXTURE_2D);

	//restore default color
	glColor4f(1.f, 1.f, 1.f, 1.f);


	auto i = (int)(noiseTexturesCountFactor * (float)(STEPS_COUNT - 1));
	this->currentTexId += steps[i];
	this->currentTexId %= NTEX_COUNT;

	return FF_SUCCESS;
}


FFGLFlows::FBOPair* FFGLFlows::CreateFBO(const GLuint width, const GLuint height)
{

	GLenum status{ GL_INVALID_FRAMEBUFFER_OPERATION };
	FBOPair* pair = new FBOPair();

	// Init evenBufferId/evenTextureId
	glGenFramebuffers(1, &pair->firstBufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, pair->firstBufferId);

	glGenTextures(1, &pair->firstTextureId);
	glBindTexture(GL_TEXTURE_2D, pair->firstTextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pair->firstTextureId, 0);

	status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		return nullptr;

	// Init oddBufferId/oddTextureId
	glGenFramebuffers(1, &pair->secondBufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, pair->secondBufferId);

	glGenTextures(1, &pair->secondTextureId);
	glBindTexture(GL_TEXTURE_2D, pair->secondTextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pair->secondTextureId, 0);

	status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
		return nullptr;

	pair->reversedDirection = false;

	return pair;

}


void FFGLFlows::DeleteNoiseTextures()
{
	if (instanceCounter == 0 && texIds.size() != 0)
	{
		glDeleteTextures(NTEX_COUNT, texIds.data());
		texIds.clear();
	}
}


void FFGLFlows::EnsureTexturesCreated(int width, int height)
{
	const int patternSize{ width * height };

	GLubyte lut[256];
	for (int i = 0; i < 256; i++) lut[i] = i < 127 ? 0 : 255;

	GLubyte *phase = new GLubyte[patternSize];
	GLubyte* pattern = new GLubyte[patternSize];


	for (int i = 0; i < patternSize; i++)
		phase[i] = rand() % 256;

	texIds.resize(NTEX_COUNT);

	glGenTextures(NTEX_COUNT, texIds.data());

	for (int i = 0; i < NTEX_COUNT; i++)
	{
		int arg{ i * 256 / NTEX_COUNT };

		for (int j = 0; j < patternSize; j++)
			pattern[j] = lut[(arg + phase[j]) % 255];

		glBindTexture(GL_TEXTURE_2D, texIds[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8, this->noiseWidth, this->noiseHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pattern);
	}

	delete phase;
	delete pattern;
}

/*
DWORD FFGLFlows::GetParameter(DWORD dwIndex)
{
	DWORD dwRet;

	auto result = parameterDefinitions.find(dwIndex);

	if (result != parameterDefinitions.end())
	{
		if (result->first == (int)ParamNames::FUNC_DEF)
		{
		}
		else
		{
			*((float *)(unsigned)(&dwRet)) = *(result->second.floatValueStorage);
			return dwRet;
		}
	}

	return FF_FAIL;

}

DWORD FFGLFlows::SetParameter(const SetParameterStruct* pParam)
{
	if (pParam != NULL)
	{
		auto result = parameterDefinitions.find(pParam->ParameterNumber);

		if (result != parameterDefinitions.end())
		{

			if (result->first == (int)ParamNames::FUNC_DEF)
			{
				if (strlen((char*)pParam->NewParameterValue) > 0)
				{
					if (this->fieldCode.compare((char*)pParam->NewParameterValue) != 0)
					{
						this->fieldCode.assign((char*)pParam->NewParameterValue);

						string tmpCode{ analyticShaderCode };

						auto pos = tmpCode.find(CALULATE_STATEMENTS);
						tmpCode.replace(pos, CALULATE_STATEMENTS.length(), fieldCode);

						analyticShader.Compile(VERTEX_SHADER.c_str(), tmpCode.c_str());
					}
				}
			}
			else
			{
				float newValue = *((float *)(unsigned)&(pParam->NewParameterValue));

				if (newValue != *(result->second.floatValueStorage))
				{
					if (result->first == (int)ParamNames::NOISES_TEXTURES_COUNT)
					{
						currentTexId = 0;
					}

					*(result->second.floatValueStorage) = newValue;
				}
			}
		}
		else
		{
			return FF_FAIL;
		}

		return FF_SUCCESS;
	}

	return FF_FAIL;
}
*/

FFResult FFGLFlows::SetFloatParameter(unsigned int dwIndex, float value)
{
	auto result = parameterDefinitions.find(dwIndex);
	if (result == parameterDefinitions.end())
		return FF_FAIL;

	if (result->second.getType() == FF_TYPE_STANDARD)
	{
		if ((float)result->second == value)
			return FF_SUCCESS;
		
		result->second.setValue(value);

		switch (result->first)
		{
			case (int)ParamNames::NOISE_FACTOR:
				this->alphaNoisesTexture = value;
				break;

			case (int)ParamNames::IMG_BLENDING_FACTOR:
				this->alphaImageTexture = value;
				break;

			case (int)ParamNames::NOISES_TEXTURES_COUNT:
				this->noiseTexturesCountFactor = value;
				break;

			case (int)ParamNames::NOISES_SCALE:
				this->noiseDimScale = value;
				break;

			case (int)ParamNames::OPERATOR_TYPE:
				this->operatorTypeFactor = value;
				break;

			case (int)ParamNames::X1:
				this->x1 = value;
				break;

			case (int)ParamNames::Y1:
				this->y1 = value;
				break;

			case (int)ParamNames::X2:
				this->x2 = value;
				break;

			case (int)ParamNames::Y2:
				this->y2 = value;
				break;

			case (int)ParamNames::X3:
				this->x3 = value;
				break;

			case (int)ParamNames::Y3:
				this->y3 = value;
				break;

			case (int)ParamNames::VELOCITY:
				this->velocity = value;
				break;

			case (int)ParamNames::VELOCITY_SCALE:
				this->velocityScale = value;
				break;
		};		
	}

	return FF_SUCCESS;
}

float FFGLFlows::GetFloatParameter(unsigned int index)
{
	auto result = parameterDefinitions.find(index);
	if (result == parameterDefinitions.end())
		return FF_FAIL;

	if (result->second.getType() == FF_TYPE_TEXT)
		return FF_FAIL;

	return result->second.getFloatStorage();
}

FFResult FFGLFlows::SetTextParameter(unsigned int index, const char *value)
{
	auto result = parameterDefinitions.find(index);
	if (result == parameterDefinitions.end())
		return FF_FAIL;

	if (result->second.getType() == FF_TYPE_TEXT)
	{
		switch (result->first)
		{
			case (int)ParamNames::FUNC_DEF:			
				string currentValue = result->second.getStrStorage();

				if (currentValue.compare(value) == 0)
					break;
				result->second.setStrStorage(value);				
				fieldCode.assign(value);
				shaderCodeChanged = true;
				break;
		}
	}

	return FF_SUCCESS;
}

char*  FFGLFlows::GetTextParameter(unsigned int index)
{
	auto result = parameterDefinitions.find(index);
	if (result == parameterDefinitions.end())
		return nullptr;

	if (result->second.getType() != FF_TYPE_TEXT)
		return nullptr;

	return (char*)result->second.getStrStorage().c_str();
}
