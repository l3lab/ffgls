#include "FFGLDelays.h"
#include "utilities/FrameBufferUtils.h"
#include "utilities/utilities.h"
#include <FFGLLib.h>


static CFFGLPluginInfo PluginInfo(
	FFGLDelays::CreateInstance,	// Create method
	"DL23",						// Plugin unique ID
	"L3.Delay",					// Plugin name											
	1,						   	// API major version number 											
	500,						// API minor version number	
	1,							// Plugin major version number
	000,						// Plugin minor version number
	FF_EFFECT,					// Plugin type
	"FFGL L3 Delay",			// Plugin description
	"L3 Lab"					// About
	);

static const std::string vertexShaderCode = STRINGIFY(
	// Vertex cordinates range [-1, 1]
	in vec3 a_vertexCoords;
// Texture cordinates range [0, 1]
in vec2 a_textureCoords;
//
uniform vec2 maxTextureCoords;
void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * vec4(a_vertexCoords.xyz, 1.0);
	gl_TexCoord[0] = vec4(a_textureCoords.s * maxTextureCoords.s,
		a_textureCoords.t * maxTextureCoords.t, 0.0, 0.0);
	gl_FrontColor = gl_Color;
});


static const std::string fragmentShaderCode = STRINGIFY(
	uniform sampler2D inputTexture;
void main()
{
	vec4 color = vec4(1.0, 0.0, 0.0, 1.0);
	color = texture2D(inputTexture, gl_TexCoord[0].st);
	gl_FragColor = color;
});

const std::string defaultCalulations{ STRINGIFY(120) };

static const std::map<int, FFGLParameter> lDefs
{
	{ (int)ParamCodes::NUM_MODULES, FFGLParameter{ "Modules num", 0.5f } },
	{ (int)ParamCodes::PATTERN_MODE , FFGLParameter{ "Pattern mode", 0.0f } },
	{ (int)ParamCodes::CONFIG_BUFFERS_NUMBER , FFGLParameter{ "Buffers count", defaultCalulations } }
};

FFGLDelays::FFGLDelays() :CFreeFrameGLPlugin()
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
		else if (value.getType() == FF_TYPE_TEXT)
			SetParamInfo(param.first, value.getName().c_str(), value.getType(), value.getStrStorage().c_str());
	}
}

FFGLDelays::~FFGLDelays()
{
	parameterDefinitions.clear();
	DeInitGL();
}


FFResult FFGLDelays::InitGL(const FFGLViewportStruct *vp)
{
	viewPoint = *vp;
	bufferCount = std::stoi(bufferNumber);	
	reConstructBuffers(viewPoint, bufferCount);

	return FF_SUCCESS;
}

FFResult FFGLDelays::DeInitGL()
{
	for (auto fb : fbos)
	{
		fb.FreeResources();
		//FFGLUtils::DeleteFrameBuffer(fb, glExts);
	};

	fbos.clear();

	return FF_SUCCESS;
}

void FFGLDelays::processParamsValuesChanges()
{
	auto numModulesParam = parameterDefinitions.find((int)ParamCodes::NUM_MODULES);
	modulesAmountCoeff = numModulesParam->second.getFloatStorage();

	auto patternModesParam = parameterDefinitions.find((int)ParamCodes::PATTERN_MODE);
	patternCoeff = patternModesParam->second.getFloatStorage();

	auto bufferNumberParam = parameterDefinitions.find((int)ParamCodes::CONFIG_BUFFERS_NUMBER);
	bufferNumber.assign(bufferNumberParam->second.getStrStorage());

	if (bufferCount != std::stoi(bufferNumber))
	{
		bufferCount = std::stoi(bufferNumber);
		reConstructBuffers(viewPoint, bufferCount);
	}

}


void FFGLDelays::reConstructBuffers(const FFGLViewportStruct vp, int puffers)
{
	index = 0;

	if (!fbos.empty())
		for (auto fb : fbos)
			fb.FreeResources();
	fbos.clear();

	for (int i = 0; i < puffers; i++)
	{
		auto fb = new FFGLFBO();
		fb->Create(viewPoint.width, viewPoint.height);

		//auto fb = FFGLUtils::CreateFrameBuffer(viewPoint->width, viewPoint->height, glExts);
		fbos.push_back(*fb);
	};
}

FFResult FFGLDelays::ProcessOpenGL(ProcessOpenGLStruct* pGL)
{
	if (pGL->numInputTextures < 1)
		return FF_FAIL;
	if (pGL->inputTextures[0] == NULL)
		return FF_FAIL;

	FFGLTextureStruct inputTexture = *(pGL->inputTextures[0]);
	FFGLTexCoords maxCoords = GetMaxGLTexCoords(inputTexture);

	processParamsValuesChanges();

	glEnable(GL_TEXTURE_2D);


	// saving current frame buffer object
	GLint currentFb;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFb);

	// render to the i-frame buffer	
	// set rendering destination to FBO

	fbos[index].BindAsRenderTarget();

	glBindFramebuffer(GL_FRAMEBUFFER, fbos[index].GetFBOHandle());
	glBindTexture(GL_TEXTURE_2D, inputTexture.Handle);
	glClear(GL_COLOR_BUFFER_BIT);

	FrameBufferUtils::FrameRect(maxCoords);

	//unbind the input texture
	glBindTexture(GL_TEXTURE_2D, 0);

	// back to the current frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, currentFb);


	const int moduleCount{ (int)(2.0f * (1.0 - modulesAmountCoeff) + modulesAmountCoeff * ((float)bufferCount)) };
	const double stripGeomWidth{ ((double)2) / ((double)moduleCount) };
	const double stripTexWigth{ ((double)1) / ((double)moduleCount) };
	const float fStep{ ((float)bufferCount) / ((float)(moduleCount - 1)) };

	float fIndex = GetOldest();

	double geomPos = -1;
	double texPos = 0;

	auto quadType{ GetQuadType() };
	
	if (quadType == VERTICAL_STRIPS || quadType == HORIZONTAL_STRIPS)
	{
		for (int i = 0; i < moduleCount; i++)
		{
			int fbIndex = (i == 0) ? GetOldest() : (i == (moduleCount - 1)) ? GetNewest() : (int)(GetOldest() + ((float)i) * fStep) % bufferCount;			
			glBindTexture(GL_TEXTURE_2D, fbos[fbIndex].GetTextureInfo().Handle);

			switch (quadType)
			{
			case VERTICAL_STRIPS:
				VStrip(geomPos, stripGeomWidth, texPos, stripTexWigth);
				break;
			case HORIZONTAL_STRIPS:
				HStrip(geomPos, stripGeomWidth, texPos, stripTexWigth);
				break;
			}
			geomPos += stripGeomWidth;
			texPos += stripTexWigth;
			
		};
	};

	if (quadType == QUAD_NET_FROM_LLC || quadType == QUAD_NET_FROM_URC)
	{
		int quadIdx{ 0 };
		auto dims = GetMultipliers(inputTexture, moduleCount);
		
		const float delthaQuadWidth{ 2.0f / ((float)dims.Width) };
		const float delthaQuadHeight{ 2.0f / ((float)dims.Height) };

		const float delthaTextWidth{ 1.0f / ((float)dims.Width) };
		const float delthaTextHeight{ 1.0f / ((float)dims.Height) };

		float quadLeftLowerCorner_x{ -1.0f };
		float quadLeftLowerCorner_y{ (quadType == Pattern::QUAD_NET_FROM_LLC) ? -1.0f : 1.0f - delthaQuadHeight };

		float texLeftLowerCorner_x{ 0.0f };
		float texLeftLowerCorner_y{ (quadType == Pattern::QUAD_NET_FROM_LLC) ? 0.0f : 1.0f - delthaTextHeight };


		for (int i{ 0 }; i < dims.Height; i++)
		{
			quadLeftLowerCorner_x = (quadType == QUAD_NET_FROM_LLC) ? -1.0f : 1.0f - delthaQuadWidth;
			texLeftLowerCorner_x = (quadType == QUAD_NET_FROM_LLC) ? 0.0f : 1.0f - delthaTextWidth;
			for (int j{ 0 }; j < dims.Width; j++)
			{
				int fbIndex = (quadIdx == 0) ? GetOldest() : (quadIdx == (moduleCount - 1)) ? GetNewest() : (int)(GetOldest() + ((float)quadIdx) * fStep) % bufferCount;
				glBindTexture(GL_TEXTURE_2D, fbos[fbIndex].GetTextureInfo().Handle);

				glBegin(GL_QUADS);

				//lower left
				glTexCoord2d(texLeftLowerCorner_x, texLeftLowerCorner_y);
				glVertex2f(quadLeftLowerCorner_x, quadLeftLowerCorner_y);

				//upper left	
				glTexCoord2d(texLeftLowerCorner_x, texLeftLowerCorner_y + delthaTextHeight);
				glVertex2f(quadLeftLowerCorner_x, quadLeftLowerCorner_y + delthaQuadHeight);

				//upper right	
				glTexCoord2d(texLeftLowerCorner_x + delthaTextWidth, texLeftLowerCorner_y + delthaTextHeight);
				glVertex2f(quadLeftLowerCorner_x + delthaQuadWidth, quadLeftLowerCorner_y + delthaQuadHeight);

				//lower right
				glTexCoord2d(texLeftLowerCorner_x + delthaTextWidth, texLeftLowerCorner_y);
				glVertex2f(quadLeftLowerCorner_x + delthaQuadWidth, quadLeftLowerCorner_y);

				glEnd();

				if (quadType == QUAD_NET_FROM_LLC)
				{
					quadLeftLowerCorner_x += delthaQuadWidth;
					texLeftLowerCorner_x += delthaTextWidth;
				}
				else
				{
					quadLeftLowerCorner_x -= delthaQuadWidth;
					texLeftLowerCorner_x -= delthaTextWidth;
				}

				quadIdx++;
			}

			if (quadType == QUAD_NET_FROM_LLC)
			{
				quadLeftLowerCorner_y += delthaQuadHeight;
				texLeftLowerCorner_y += delthaTextHeight;
			}
			else
			{
				quadLeftLowerCorner_y -= delthaQuadHeight;
				texLeftLowerCorner_y -= delthaTextHeight;
			}
		}
	}	

	IncIndex();

	glDisable(GL_TEXTURE_2D);

	return FF_SUCCESS;
}


FFResult FFGLDelays::SetFloatParameter(unsigned int dwIndex, float value)
{
	auto result = parameterDefinitions.find(dwIndex);
	if (result == parameterDefinitions.end())
		return FF_FAIL;

	if (result->second.getType() == FF_TYPE_STANDARD)
	{
		if ((float)result->second == value)
			return FF_SUCCESS;

		result->second.setValue(value);
	}

	return FF_SUCCESS;
}

float FFGLDelays::GetFloatParameter(unsigned int index)
{
	auto result = parameterDefinitions.find(index);
	if (result == parameterDefinitions.end())
		return FF_FAIL;

	if (result->second.getType() == FF_TYPE_TEXT)
		return FF_FAIL;

	return result->second.getFloatStorage();
}

FFResult FFGLDelays::SetTextParameter(unsigned int index, const char *value)
{
	auto result = parameterDefinitions.find(index);
	if (result == parameterDefinitions.end())
		return FF_FAIL;

	if (result->second.getType() == FF_TYPE_TEXT)
		result->second.setStrStorage(value);
	
	return FF_SUCCESS;
}

char*  FFGLDelays::GetTextParameter(unsigned int index)
{
	auto result = parameterDefinitions.find(index);
	if (result == parameterDefinitions.end())
		return nullptr;

	if (result->second.getType() != FF_TYPE_TEXT)
		return nullptr;

	return (char*)result->second.getStrStorage().c_str();
}



static void VStrip(double tPos, double height, double lTexPos, double texWidth)
{
	glBegin(GL_QUADS);

	//lower left
	glTexCoord2d(0.0, lTexPos);
	glVertex2f(-1, tPos);

	//upper left	
	glTexCoord2d(0.0, lTexPos + texWidth);
	glVertex2f(-1, tPos + height);

	//upper right	
	glTexCoord2d(1.0, lTexPos + texWidth);
	glVertex2f(1, tPos + height);

	//lower right
	glTexCoord2d(1.0, lTexPos);
	glVertex2f(1, tPos);

	glEnd();

}

static void HStrip(double lPos, double width, double lTexPos, double texWidth)
{
	glBegin(GL_QUADS);

	//lower left
	glTexCoord2d(lTexPos, 0.0);
	glVertex2f(lPos, -1);

	//upper left	
	glTexCoord2d(lTexPos, 1);
	glVertex2f(lPos, 1);

	//upper right	
	glTexCoord2d(lTexPos + texWidth, 1);
	glVertex2f(lPos + width, 1);

	//lower right
	glTexCoord2d(lTexPos + texWidth, 0.0);
	glVertex2f(lPos + width, -1);

	glEnd();
}

static FFGLTextureStruct GetMultipliers(FFGLTextureStruct textureDesc, int modulesCount)
{
	const float ratio { (float)textureDesc.HardwareHeight / (float)textureDesc.HardwareWidth };

	DWORD n{ (DWORD)sqrt(modulesCount) };
	DWORD m{ (DWORD)(ratio * (float)n) };

	DWORD foundN{ 0 };
	DWORD foundM{ 0 };

	while (n*m <= modulesCount)
	{
		foundN = n;
		foundM = m;
		m = (DWORD)(ratio * (float)(++n));
	}

	auto nextN{ foundN + 1 };
	auto nextM{ foundM + 1 };
	
	if (nextN * nextM <= modulesCount)
	{
		return FFGLTextureStruct{ nextN, nextM };
	}
	else
	{
		auto m1{ nextN * foundM };
		auto m2{ foundN * nextM };

		if (m1 < m2 && m2 <= modulesCount)
			return FFGLTextureStruct{ foundN, nextM };

		if (m2 < m1 && m1 <= modulesCount)
			return FFGLTextureStruct{ nextN, foundM };
	}
	
	return FFGLTextureStruct{ foundN, foundM };
}
