#include <FFGL.h>
#include <FFGLLib.h>

#include "FFGLMosaicMixer.h"

// Parameters
#define	FFPARAM_Blend		(0)


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo(
	FFGLMosaicMixer::CreateInstance,	// Create method
	"MM101",					// Plugin unique ID
	"MosaicMixer",				// Plugin name											
	1,						   	// API major version number 											
	000,						// API minor version number	
	1,							// Plugin major version number
	000,						// Plugin minor version number
	FF_EFFECT,					// Plugin type
	"FFGL Mosaic",				// Plugin description
	"by Oleg Potiy"				// About
	);

static const GLfloat vertexQuad[4][3] = {
	{ -1.0, -1.0,  0.0 },
	{ 1.0,  -1.0,  0.0 },
	{ 1.0,   1.0,  0.0 },
	{ -1.0,  1.0,  0.0 }
};

static const GLfloat vertexQuadTexCoords[4][2] = {
	{ 0, 0 },
	{ 1, 0 },
	{ 1, 1 },
	{ 0, 1 }
};

static const std::string vertexShaderCode = STRINGIFY(
// Vertex cordinates range [-1, 1]
in vec3 a_vertexCoords;
// Texture cordinates range [0, 1]
in vec2 a_textureCoords;
// Predefined shift direction 
in vec2 a_vertexDirections;

uniform vec2 shiftAmount;
uniform vec2 maxTextureCoords;

void main()
{
	float horisontalShift = a_vertexDirections.x * shiftAmount.x;
	float verticalShift = a_vertexDirections.y * shiftAmount.y;
	
	gl_Position = gl_ModelViewProjectionMatrix * vec4(a_vertexCoords.xyz, 1.0) + vec4(horisontalShift, verticalShift, 0.0, 0.0);
	gl_TexCoord[0] = vec4(a_textureCoords.s * maxTextureCoords.s, a_textureCoords.t * maxTextureCoords.t, 0.0, 0.0) +
		0.5 * vec4(horisontalShift * maxTextureCoords.s, verticalShift * maxTextureCoords.t, 0.0, 0.0);

	gl_FrontColor = gl_Color;
});

static const std::string fragmentShaderCode = STRINGIFY(
	uniform sampler2D inputTexture;
void main()
{
	gl_FragColor = texture2D(inputTexture, gl_TexCoord[0].st);
});



////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////

FFGLMosaicMixer::FFGLMosaicMixer() :CFreeFrameGLPlugin()
{
	// Input properties
	SetMinInputs(2);
	SetMaxInputs(2);

	// parameters:
	SetParamInfo(FFPARAM_Blend, "Blend", FF_TYPE_STANDARD, 0.5f);
	m_mixerValue = 0.5f;
}

FFGLMosaicMixer::~FFGLMosaicMixer()
{
}



void FFGLMosaicMixer::BuildGeometry(int quadCount)
{
	
	GLint vertexCoordsAttrId{ attributeValues["a_vertexCoords"] };
	GLint texCoordsAttrId{ attributeValues["a_textureCoords"] };
	GLint vertexDirectionsAttrId{ attributeValues["a_vertexDirections"] };
	
	

	if (!m_isBillboardQuadConstructed)
	{
		glGenVertexArrays(1, &m_vaoBillboardQuad);
		// Bind our Vertex Array Object as the current used object
		glBindVertexArray(m_vaoBillboardQuad);


		// Allocate and assign two Vertex Buffer Objects to our handle
		GLuint vbo[3];
		glGenBuffers(3, vbo);

		// Bind our first VBO as being the active buffer and storing vertex attributes (coordinates)
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, 4 * 3 * sizeof(GLfloat), vertexQuad, GL_STATIC_DRAW);
		glVertexAttribPointer(vertexCoordsAttrId, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(vertexCoordsAttrId);

		// Bind our first VBO as being the active buffer and storing vertex attributes (tex coords)
		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat), vertexQuadTexCoords, GL_STATIC_DRAW);
		glVertexAttribPointer(texCoordsAttrId, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(texCoordsAttrId);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat), vertexQuadTexCoords, GL_STATIC_DRAW);
		glVertexAttribPointer(vertexDirectionsAttrId, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(vertexDirectionsAttrId);	

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		m_isBillboardQuadConstructed = true;
	}

	
	std::vector<GLVertex> randomQuadsVertices(4 * quadCount);
	std::vector<GLTexcoords> randomQuadsTexCoords(4 * quadCount);
	std::vector<GLTexcoords> randomQuadsShiftDirections(4 * quadCount);
	GLVertex centerTexPoint;	
	

	for (int i = 0; i < quadCount; i++)
	{
		GLVertex invPoint{ (float)rand() / (float)(RAND_MAX + 1),
							(float)rand() / (float)(RAND_MAX + 1),
							.5f * (float)i / (float)quadCount };

		GLVertex centerPoint{ 
			2.0f * invPoint.x - 1.0f, 
			2.0f * invPoint.y - 1.0f, 
			2.0f * invPoint.z - 1.0f };

		float quadSideHalf{ .25f * (float)rand() / (float)(RAND_MAX + 1) };
		float quaterSize{ .5f * quadSideHalf };

		int idx{ i * 4 };		
		GLTexcoords quadShiftDirection = shiftDirections[i % 4];
		GLVertex v0{ centerPoint.x - quadSideHalf ,centerPoint.y - quadSideHalf, centerPoint.z };
		GLTexcoords t0{ invPoint.x - quaterSize, invPoint.y - quaterSize };		
		randomQuadsVertices[idx] = v0;
		randomQuadsTexCoords[idx] = t0;
		randomQuadsShiftDirections[idx++] = quadShiftDirection;

		GLVertex v1{ centerPoint.x + quadSideHalf ,centerPoint.y - quadSideHalf, centerPoint.z };
		GLTexcoords t1{ invPoint.x + quaterSize, invPoint.y - quaterSize};		
		randomQuadsVertices[idx] = v1;
		randomQuadsTexCoords[idx] = t1;
		randomQuadsShiftDirections[idx++] = quadShiftDirection;

		GLVertex v2{ centerPoint.x + quadSideHalf ,centerPoint.y + quadSideHalf, centerPoint.z };
		GLTexcoords t2{ invPoint.x + quaterSize, invPoint.y + quaterSize };		
		randomQuadsVertices[idx] = v2;
		randomQuadsTexCoords[idx] = t2;
		randomQuadsShiftDirections[idx++] = quadShiftDirection;

		GLVertex v3{ centerPoint.x - quadSideHalf ,centerPoint.y + quadSideHalf, centerPoint.z };
		GLTexcoords t3{ invPoint.x - quaterSize, invPoint.y + quaterSize };		
		randomQuadsVertices[idx] = v3;
		randomQuadsTexCoords[idx] = t3;
		randomQuadsShiftDirections[idx++] = quadShiftDirection;
	};
	

	if (!m_areRandomQuadsConstructed)
	{
		glGenVertexArrays(1, &m_vaoBillboardRandomQuads);
		glBindVertexArray(m_vaoBillboardRandomQuads);
		glGenBuffers(3, m_vboBillboardRandomQuads);

		// Coordinates
		glBindBuffer(GL_ARRAY_BUFFER, m_vboBillboardRandomQuads[0]);
		glBufferData(GL_ARRAY_BUFFER, quadCount * 4 * 3 * sizeof(GLfloat), randomQuadsVertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(vertexCoordsAttrId, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(vertexCoordsAttrId);

		// Tex coords
		glBindBuffer(GL_ARRAY_BUFFER, m_vboBillboardRandomQuads[1]);
		glBufferData(GL_ARRAY_BUFFER, quadCount * 4 * 2 * sizeof(GLfloat), randomQuadsTexCoords.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(texCoordsAttrId, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(texCoordsAttrId);

		// Shift dirs
		glBindBuffer(GL_ARRAY_BUFFER, m_vboBillboardRandomQuads[2]);
		glBufferData(GL_ARRAY_BUFFER, quadCount * 4 * 2 * sizeof(GLfloat), randomQuadsShiftDirections.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(vertexDirectionsAttrId, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(vertexDirectionsAttrId);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		m_areRandomQuadsConstructed = true;
	}
	else
	{
		glBindVertexArray(m_vaoBillboardRandomQuads);
		// Coordinates
		glBindBuffer(GL_ARRAY_BUFFER, m_vboBillboardRandomQuads[0]);
		glBufferData(GL_ARRAY_BUFFER, quadCount * 4 * 3 * sizeof(GLfloat), randomQuadsVertices.data(), GL_STATIC_DRAW);

		// Tex coords
		glBindBuffer(GL_ARRAY_BUFFER, m_vboBillboardRandomQuads[1]);
		glBufferData(GL_ARRAY_BUFFER, quadCount * 4 * 2 * sizeof(GLfloat), randomQuadsTexCoords.data(), GL_STATIC_DRAW);

		// Shift dirs
		glBindBuffer(GL_ARRAY_BUFFER, m_vboBillboardRandomQuads[2]);
		glBufferData(GL_ARRAY_BUFFER, quadCount * 4 * 2 * sizeof(GLfloat), randomQuadsShiftDirections.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	randomQuadsVertices.clear();
	randomQuadsTexCoords.clear();
	randomQuadsShiftDirections.clear();
	
	m_areRandomQuadsConstructed = true;
	m_quadCount = quadCount;
}


FFResult	FFGLMosaicMixer::InitGL(const FFGLViewportStruct *vp)
{
	

	m_shader.Compile(vertexShaderCode, fragmentShaderCode, attributeValues);
	m_shader.BindShader();

	auto m_inputTextureLocation{ m_shader.FindUniform("inputTexture") };
	glUniform1i(m_inputTextureLocation, 0);

	m_shader.UnbindShader();	

	return FF_SUCCESS;
}

FFResult	FFGLMosaicMixer::DeInitGL()
{
	return FF_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////
const float leftBorderLastsTill{ 1.0f / 3.0F };
const float rightBorderStartsFrom{ 1.0f - leftBorderLastsTill };
const float borderRatio{ 1.0f / leftBorderLastsTill };
const int objectsCount{ 100 };

FFResult FFGLMosaicMixer::ProcessOpenGL(ProcessOpenGLStruct *pGL)
{
	GLint currentVao{ 0 };
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVao);

	auto maxTexCoordsParamLocation{ m_shader.FindUniform("maxTextureCoords") };
	auto shiftParamLocation{ m_shader.FindUniform("shiftAmount") };

	FFGLTextureStruct TextureObject1 = *(pGL->inputTextures[0]);
	FFGLTextureStruct TextureObject2 = *(pGL->inputTextures[1]);

	FFUInt32 frameHeight{ pGL->inputTextures[0]->HardwareHeight };
	FFUInt32 frameWidth{ pGL->inputTextures[0]->HardwareWidth };

	FFGLTexCoords maxCoords = GetMaxGLTexCoords(TextureObject1);

	if (TextureObject1.Width < TextureObject2.Width)
	{
		maxCoords = GetMaxGLTexCoords(TextureObject2);
		frameHeight = pGL->inputTextures[1]->Height;
		frameWidth = pGL->inputTextures[1]->Width;
	}

	m_shader.BindShader();
	
	
	// Update VAOs if needed
	if (m_updateGeometryFlag)
	{		
		glUniform2f(maxTexCoordsParamLocation, maxCoords.s, maxCoords.t);
		glUniform2f(shiftParamLocation, 0.0f, 0.0f);
		this->BuildGeometry(objectsCount);	
		m_updateGeometryFlag = false;
	}
	
	
	
	glBindTexture(GL_TEXTURE_2D, TextureObject1.Handle);	
	glUniform2f(maxTexCoordsParamLocation, maxCoords.s, maxCoords.t);
	glUniform2f(shiftParamLocation, 0.0f, 0.0f);	
	glBindVertexArray(m_vaoBillboardQuad);
	glDrawArrays(GL_QUADS, 0, 4);	

	if (m_mixerValue < 1)
	{
		float boundarySmoother{ 1.0f };

		if (m_mixerValue < leftBorderLastsTill)
		{
			boundarySmoother = borderRatio * m_mixerValue;
		}
		else if (m_mixerValue > rightBorderStartsFrom)
		{
			boundarySmoother = borderRatio * (1.0f - m_mixerValue);
		}

		if (m_frameCounter > objectsCount)
		{
			m_frameCounter = 0;
			m_dirSwitchCounter += 1;			
			m_shiftDirection = directions[m_dirSwitchCounter % 4];
			//m_updateGeometryFlag = true;
			m_shift = 0;
		}
		
		//m_frameShift.s += 0.001f;
		//m_frameShift.t += 0.001f;
		m_shift += 0.001f;
		
		
		glBindTexture(GL_TEXTURE_2D, TextureObject2.Handle);		
		glUniform2f(maxTexCoordsParamLocation, maxCoords.s, maxCoords.t);
		glUniform2f(shiftParamLocation, m_shift * boundarySmoother * m_shiftDirection.s , m_shift * boundarySmoother * m_shiftDirection.t);
		glBindVertexArray(m_vaoBillboardRandomQuads);
		glDrawArrays(GL_QUADS, 0, 4 * m_quadCount * m_mixerValue );								
		m_frameCounter += 1;
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, TextureObject2.Handle);
		glUniform2f(shiftParamLocation, .0f, .0f);
		glBindVertexArray(m_vaoBillboardQuad);
		glDrawArrays(GL_QUADS, 0, 4);
		
	}

	m_shader.UnbindShader();
	glDisable(GL_TEXTURE_2D);

	glBindVertexArray(currentVao);

	return FF_SUCCESS;
}



float FFGLMosaicMixer::GetFloatParameter(unsigned int dwIndex)
{
	return m_mixerValue;
}

FFResult FFGLMosaicMixer::SetFloatParameter(unsigned int dwIndex, float value)
{
	if (m_mixerValue != 1.0 && value == 1.0)
	{
		m_updateGeometryFlag = true;
	}
	m_mixerValue = value;
	return FF_SUCCESS;
}


