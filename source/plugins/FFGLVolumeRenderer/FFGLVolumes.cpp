#include <FFGL.h>
#include <FFGLLib.h>
#include <gl\GLU.h>

#include "FFGLVolumes.h"
#include "../../lib/ffgl/utilities/utilities.h"

#define USE_VBO



////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo(
	FFGLVolumeRendering::CreateInstance,	// Create method
	"VOLR",								// Plugin unique ID
	"FFGLVolume",     // Plugin name											
	1,						   			// API major version number 											
	500,								  // API minor version number	
	1,										// Plugin major version number
	000,									// Plugin minor version number
	FF_EFFECT,						// Plugin type
	"FFGL Volume rendering plug",	// Plugin description
	"by Oleg Potiy" // About
	);


static const int MaxPlanes = 200;


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

enum class Params : int
{
	IS_PERSPECTIVE,
	SCALE,
	X_ANGLE,
	Y_ANGLE,
	PLANES_DISTANCE,
	PLANES_COUNT,
	FOVY
};

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

static const std::map<int, FFGLParameter> lDefs
{
	{ (int)Params::IS_PERSPECTIVE, FFGLParameter{ "Perspective", false } },
	{ (int)Params::SCALE, FFGLParameter{ "Scale", 1.0f } },
	{ (int)Params::X_ANGLE , FFGLParameter{ "Angle (X)", 0.5f } },
	{ (int)Params::Y_ANGLE , FFGLParameter{ "Angle (Y)", 0.5f } },
	{ (int)Params::PLANES_DISTANCE, FFGLParameter{ "Planes distance", 1.0f } },
	{ (int)Params::PLANES_COUNT, FFGLParameter{ "Planes count", 0.5f } },
	{ (int)Params::FOVY, FFGLParameter{ "FOVY", 0.0f } },
};

FFGLVolumeRendering::FFGLVolumeRendering() :CFreeFrameGLPlugin()
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

	this->isGeometryRebuildNeeded = true;
	this->VertexData = NULL;
	this->TexcoordData = NULL;

}

FFGLVolumeRendering::~FFGLVolumeRendering()
{
	parameterDefinitions.clear();
}




FFResult FFGLVolumeRendering::InitGL(const FFGLViewportStruct *vp)
{

	int complationResult = m_shader.Compile(vertexShaderCode, fragmentShaderCode, attributeValues);
	m_shader.BindShader();

	glUniform1i(m_shader.FindUniform("inputTexture"), 0);

	m_shader.UnbindShader();

	return FF_SUCCESS;
}

FFResult FFGLVolumeRendering::DeInitGL()
{
	return FF_SUCCESS;
}


void FFGLVolumeRendering::BuildGeometry(int stackQuadCount, float stackSize)
{
	GLint vertexCoordsAttrId{ attributeValues["a_vertexCoords"] };
	GLint texCoordsAttrId{ attributeValues["a_textureCoords"] };

	const float gapSize{ stackSize / ((stackQuadCount > 1) ? stackQuadCount - 1 : 1) };

	std::vector<GlVertex> stackingQuadsVertices(4 * stackQuadCount);
	std::vector<GLTexcoords> stackingQuadsTexCoords(4 * stackQuadCount);

	for (auto i = 0; i < stackQuadCount; i++)
	{

		auto quadDistance{ -stackSize / 2 + i*gapSize };
		int idx{ i * 4 };

		stackingQuadsVertices[idx] = { -1.0, -1.0, quadDistance };
		stackingQuadsTexCoords[idx++] = { 0.0,0.0 };

		stackingQuadsVertices[idx] = { -1.0, 1.0, quadDistance };
		stackingQuadsTexCoords[idx++] = { 0.0,1.0 };

		stackingQuadsVertices[idx] = { 1.0, 1.0, quadDistance };
		stackingQuadsTexCoords[idx++] = { 1.0,1.0 };

		stackingQuadsVertices[idx] = { 1.0, -1.0, quadDistance };
		stackingQuadsTexCoords[idx++] = { 1.0,0.0 };
	};

	if (!m_isGeometryConstructed)
	{
		glGenVertexArrays(1, &m_vaoGeometryQuads);
		glBindVertexArray(m_vaoGeometryQuads);
		glGenBuffers(2, m_vboGeometryQuads);

		// Filling buffer with vertex data
		glBindBuffer(GL_ARRAY_BUFFER, m_vboGeometryQuads[0]);
		glBufferData(GL_ARRAY_BUFFER, stackQuadCount * 4 * 3 * sizeof(GLfloat), stackingQuadsVertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(vertexCoordsAttrId, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(vertexCoordsAttrId);

		// Filling buffer with texcoords
		glBindBuffer(GL_ARRAY_BUFFER, m_vboGeometryQuads[1]);
		glBufferData(GL_ARRAY_BUFFER, stackQuadCount * 4 * 2 * sizeof(GLfloat), stackingQuadsTexCoords.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(texCoordsAttrId, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(texCoordsAttrId);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	else
	{
		glBindVertexArray(m_vaoGeometryQuads);
		// Coordinates
		glBindBuffer(GL_ARRAY_BUFFER, m_vboGeometryQuads[0]);
		glBufferData(GL_ARRAY_BUFFER, stackQuadCount * 4 * 3 * sizeof(GLfloat), stackingQuadsVertices.data(), GL_STATIC_DRAW);

		// Tex coords
		glBindBuffer(GL_ARRAY_BUFFER, m_vboGeometryQuads[1]);
		glBufferData(GL_ARRAY_BUFFER, stackQuadCount * 4 * 2 * sizeof(GLfloat), stackingQuadsTexCoords.data(), GL_STATIC_DRAW);
	}

	stackingQuadsVertices.clear();
	stackingQuadsTexCoords.clear();

	m_isGeometryConstructed = true;
	m_quadCount = stackQuadCount;

}




FFResult FFGLVolumeRendering::ProcessOpenGL(ProcessOpenGLStruct *pGL)
{
	GLint currentVao{ 0 };
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVao);

	auto maxTexCoordsParamLocation{ m_shader.FindUniform("maxTextureCoords") };
	
	const int quadCount{ (int)(MaxPlanes * GetFloatParameter((int)Params::PLANES_COUNT)) };
	const float stackSize{ GetFloatParameter((int)Params::PLANES_DISTANCE) };


	if (pGL->numInputTextures < 1)
		return FF_FAIL;

	if (pGL->inputTextures[0] == NULL)
		return FF_FAIL;

	FFGLTextureStruct &Texture = *(pGL->inputTextures[0]);
	//get the max s,t that correspond to the 
	//width,height of the used portion of the allocated texture space
	FFGLTexCoords maxCoords = GetMaxGLTexCoords(Texture);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	
	// Setting parallel projection 
	float perspectiveValue{ GetFloatParameter((int)Params::IS_PERSPECTIVE) };
	if (perspectiveValue > 0)
	{
		float fovy{ GetFloatParameter((int)Params::FOVY)};
		gluPerspective(90.0*(fovy + 0.5), 1, 0.1, 100.0);
		gluLookAt(0, 0, 1.5, 0, 0, 0, 0, 1, 0);		
	}
	else
	{
		glOrtho(-1, 1, -1, 1, -2, 2);
	}

#pragma region Model/View transforms
	// Setting up model transformations ...
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	// ... rotation 
	glRotatef((GetFloatParameter((int)Params::X_ANGLE) - 0.5) * 180.0, 1.0, 0.0, 0.0);
	glRotatef((GetFloatParameter((int)Params::Y_ANGLE) - 0.5) * 180.0, 0.0, 1.0, 0.0);
	// ...and scale
	float modelScale{ GetFloatParameter((int)Params::SCALE) };
	glScalef(modelScale, modelScale, modelScale);
#pragma endregion



	//enable texturemapping
	glEnable(GL_TEXTURE_2D);

	m_shader.BindShader();

	//bind the texture handle
	glBindTexture(GL_TEXTURE_2D, Texture.Handle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glUniform2f(maxTexCoordsParamLocation, maxCoords.s, maxCoords.t);

	if (m_updateGeometryFlag)
	{
		this->BuildGeometry(quadCount, stackSize);
		m_updateGeometryFlag = false;
	}

	glBindVertexArray(m_vaoGeometryQuads);
	glDrawArrays(GL_QUADS, 0, 4 * quadCount);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	
	
	

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	//unbind the texture
	glBindTexture(GL_TEXTURE_2D, 0);

	//disable texturemapping
	glDisable(GL_TEXTURE_2D);

	//disable blending
	glDisable(GL_BLEND);

	glDisable(GL_TEXTURE_2D);
	glBindVertexArray(currentVao);

	m_shader.UnbindShader();

	return FF_SUCCESS;
}


FFResult FFGLVolumeRendering::SetFloatParameter(unsigned int dwIndex, float value)
{

	auto result = parameterDefinitions.find(dwIndex);
	if (result == parameterDefinitions.end())
		return FF_FAIL;

	if (result->second.getType() != FF_TYPE_TEXT)
	{
		if (result->second.getFloatStorage() != value)
		{
			result->second.setValue(value);
			if (dwIndex == (int)Params::PLANES_DISTANCE
				|| dwIndex == (int)Params::PLANES_COUNT)
			{
				m_updateGeometryFlag = true;
			}
		}
	}

	return FF_SUCCESS;
}

float FFGLVolumeRendering::GetFloatParameter(unsigned int index)
{
	auto result = parameterDefinitions.find(index);
	if (result == parameterDefinitions.end())
		return FF_FAIL;

	if (result->second.getType() == FF_TYPE_TEXT)
		return FF_FAIL;

	return result->second.getFloatStorage();
}



