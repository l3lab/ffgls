#include <FFGL.h>
#include <FFGLLib.h>

#include "FFGLTriangleMixer.h"
#include "../../lib/ffgl/utilities/utilities.h"

#define	FFPARAM_Blend		(0)


static CFFGLPluginInfo PluginInfo ( 
	FFGLTriangleMixer::CreateInstance,	// Create method
	"TriX",							// Plugin unique ID
	"Triangle MX",						// Plugin name											
	1,						   			// API major version number 											
	000,								// API minor version number	
	1,									// Plugin major version number
	000,								// Plugin minor version number
	FF_EFFECT,							// Plugin type
	"FFGL Triangle MX",				// Plugin description
	"by Oleg Potiy"						// About
	);

FFGLTriangleMixer::FFGLTriangleMixer(void):CFreeFrameGLPlugin()
{
	// Input properties
	SetMinInputs(2);
	SetMaxInputs(2);

	// parameters:
	SetParamInfo(FFPARAM_Blend, "Blend", FF_TYPE_STANDARD, 0.5f);	
	m_blend = 0.5f;
}


FFGLTriangleMixer::~FFGLTriangleMixer(void)
{
}


float FFGLTriangleMixer::GetFloatParameter(unsigned int dwIndex)
{	
	return this->m_blend;
}

FFResult FFGLTriangleMixer::SetFloatParameter(unsigned int dwIndex, float value)
{
	this->m_blend = value;
	this->pattern = (TexPattern)(unsigned char)(this->m_blend * (TexPattern::All + 0.9));
	return FF_SUCCESS;
}

FFResult FFGLTriangleMixer::ProcessOpenGL(ProcessOpenGLStruct* pGL)
{
	if (pGL->numInputTextures < 2) 
		return FF_FAIL;

	if (pGL->inputTextures[0]==NULL || pGL->inputTextures[1]==NULL ) 
		return FF_FAIL;
	
	m_shader.BindShader();

	this->ScalableTriangleMeshMix(pGL);

	m_shader.UnbindShader();

	return FF_SUCCESS;
}



const static double divisor = 1.7320508075688772935274463415059L;

DWORD FFGLTriangleMixer::ScalableTriangleMeshMix(ProcessOpenGLStruct* pGL)
{
	int i{ 0 };
	FFGLTextureStruct TextureObject1 = *(pGL->inputTextures[0]);
	FFGLTextureStruct TextureObject2 = *(pGL->inputTextures[1]);

	FFGLTexCoords maxCoords = GetMaxGLTexCoords(TextureObject1);
	DWORD frameHeight{ pGL->inputTextures[0]->Height };
	DWORD frameWidth { pGL->inputTextures[0]->Width };

	if (TextureObject1.Width < TextureObject2.Width)
	{
		maxCoords = GetMaxGLTexCoords(TextureObject2);
		frameHeight = pGL->inputTextures[1]->Height;
		frameWidth = pGL->inputTextures[1]->Width;
	}

	double halfOfS{ maxCoords.s / 2 };
	double halfOfT{ maxCoords.t / 2 };

	double triangleSide{ frameHeight / divisor };
	double halfTriangleSide{ triangleSide / 2.0 };
	
	double nrmTriangleSide{ triangleSide / (double)frameWidth };
	double nrmHalfTriangleSide{ halfTriangleSide / (double)frameWidth };

	double leftTexBorder = 0.5 - frameHeight / (frameWidth*divisor);
	double rightTexBorder = 0.5 + frameHeight / (frameWidth*divisor);
	
	glShadeModel(GL_SMOOTH);
	

	// Two central triangles
	if (this->pattern == TexPattern::SplitAsGeometry)
	{		
		// /\/\/+\/\/\		
		// \/\/\|/\/\/		
		glBindTexture(GL_TEXTURE_2D, TextureObject2.Handle);
		glBegin(GL_TRIANGLES);

		glTexCoord2d(halfOfS - nrmHalfTriangleSide, halfOfT);		
		glVertex3d(-nrmTriangleSide, 0.0, 0.0);

		glTexCoord2d(halfOfS + nrmHalfTriangleSide, halfOfT);		
		glVertex3d(nrmTriangleSide, 0.0, 0.0);

		glTexCoord2d(halfOfS, 0);		
		glVertex3d(0.0, -1.0, 0.0);
		glEnd();		
		
		// /\/\/|\/\/\		
		// \/\/\+/\/\/
		glBindTexture(GL_TEXTURE_2D, TextureObject1.Handle);
		glBegin(GL_TRIANGLES);

		glTexCoord2d(halfOfS - nrmHalfTriangleSide, halfOfT);
		glVertex3d(-nrmTriangleSide, 0.0, 0.0);

		glTexCoord2d(halfOfS, maxCoords.t);
		glVertex3d(0.0, 1.0, 0.0);

		glTexCoord2d(halfOfS + nrmHalfTriangleSide, halfOfT);
		glVertex3d(nrmTriangleSide, 0.0, 0.0);
		glEnd();
	}
	else
	{
		// /\/\/+\/\/\		
		// \/\/\|/\/\/
		glBindTexture(GL_TEXTURE_2D, TextureObject2.Handle);
		glBegin(GL_TRIANGLES);

		glTexCoord2f(leftTexBorder, maxCoords.t);
		glVertex3f(-nrmTriangleSide, 0.0, 0.0);

		glTexCoord2f(rightTexBorder, maxCoords.t);
		glVertex3f(nrmTriangleSide, 0.0, 0.0);

		glTexCoord2f(halfOfS, 0.0);
		glVertex3f(0.0, -1.0, 0.0);

		glEnd();

		// /\/\/|\/\/\		
		// \/\/\+/\/\/
		glBindTexture(GL_TEXTURE_2D, TextureObject1.Handle);
		glBegin(GL_TRIANGLES);

		glTexCoord2f(leftTexBorder, 0.0);
		glVertex3f(-nrmTriangleSide, 0.0, 0.0);

		glTexCoord2f(halfOfS, maxCoords.t);
		glVertex3f(0.0, 1.0, 0.0);

		glTexCoord2f(rightTexBorder, 0.0);
		glVertex3f(nrmTriangleSide, 0.0, 0.0);

		glEnd();		
	}

	glBindTexture(GL_TEXTURE_2D, TextureObject1.Handle);
	glBegin(GL_TRIANGLES);

	double localShift{ .0 };
	int triNum = (int)((frameWidth / 2. - halfTriangleSide) / triangleSide) + 1;
	for (i = 0 ; i < triNum ; i ++)
	{
		
		if (this->pattern == TexPattern::All )
		{
			// /\/\/|\/ \/ \
			// \/\/\|/\+/\+/					
			localShift = nrmHalfTriangleSide + i*nrmTriangleSide; 
			glTexCoord2f(leftTexBorder, 0.0);
			glVertex3f( 2.*localShift , 0.0, 0.0);		

			localShift = (i+1)*nrmTriangleSide;
			glTexCoord2f(halfOfS, maxCoords.t);
			glVertex3f(2.*localShift, 1.0, 0.0);

			localShift = nrmHalfTriangleSide + (i+1)*nrmTriangleSide; 
			glTexCoord2f(rightTexBorder, 0.0);
			glVertex3f(2.*localShift, 0.0, 0.0);			
			
			// / \/ \/|\/\/\
			// \+/\+/\|/\/\/					
			localShift = nrmHalfTriangleSide + i*nrmTriangleSide; 
			glTexCoord2f(rightTexBorder, 0.0);
			glVertex3f( -2.*localShift, 0.0, 0.0);		

			localShift = (i+1)*nrmTriangleSide; 
			glTexCoord2f(halfOfS, maxCoords.t);
			glVertex3f(-2.*localShift, 1.0, 0.0);

			localShift = nrmHalfTriangleSide + (i+1)*nrmTriangleSide; 
			glTexCoord2f(leftTexBorder, 0.0);
			glVertex3f(-2.*localShift, 0.0, 0.0);			
		}
		else
		{
			// /\/\/|\/ \/ \
			// \/\/\|/\+/\+/
			localShift = nrmHalfTriangleSide + i*nrmTriangleSide; 
			glTexCoord2f(halfOfS + localShift, halfOfT);
			glVertex3f( 2.*localShift , 0.0, 0.0);		

			localShift = (i+1)*nrmTriangleSide;
			glTexCoord2f(halfOfS + localShift, maxCoords.t);
			glVertex3f(2.*localShift, 1.0, 0.0);

			localShift = nrmHalfTriangleSide + (i+1)*nrmTriangleSide; 
			glTexCoord2f(halfOfS + localShift, halfOfT);
			glVertex3f(2.*localShift, 0.0, 0.0);

			// / \/ \/|\/\/\
			// \+/\+/\|/\/\/								
			localShift = nrmHalfTriangleSide + i*nrmTriangleSide;
			glTexCoord2f(halfOfS - localShift, halfOfT);
			glVertex3f( -2.*localShift, 0.0, 0.0);		

			localShift = (i+1)*nrmTriangleSide; 
			glTexCoord2f(halfOfS - localShift, maxCoords.t);
			glVertex3f(-2.*localShift, 1.0, 0.0);

			localShift = nrmHalfTriangleSide + (i+1)*nrmTriangleSide; 
			glTexCoord2f(halfOfS - localShift, halfOfT);
			glVertex3f(-2.*localShift, 0.0, 0.0);			
		}	
		
		// Nearest (to central ones) upper left and right triangles		
		if ((i == 0 && this->pattern == TexPattern::WithAdjacents) || this->pattern == TexPattern::All)
		{						
			// /\/\/|\+/\+/\
			// \/\/\|/ \/ \/
			localShift = i * nrmTriangleSide;
			glTexCoord2f(leftTexBorder, 0.0);
			glVertex3f(2.*localShift, -1., 0.0);

			localShift = nrmHalfTriangleSide + i*nrmTriangleSide;
			glTexCoord2f(halfOfS, maxCoords.t);
			glVertex3f(2.*localShift, 0.0, 0.0);

			localShift = (i + 1)*nrmTriangleSide;
			glTexCoord2f(rightTexBorder, 0.0);
			glVertex3f(2.*localShift, -1., 0.0);
			
			// /\+/\+/|\/\/\		
			// \/ \/ \|/\/\/		
			localShift = (i + 1)*nrmTriangleSide;
			glTexCoord2f(leftTexBorder, 0.0);
			glVertex3f(-2.*localShift, -1.0, 0.0);

			localShift = nrmHalfTriangleSide + i*nrmTriangleSide;
			glTexCoord2f(halfOfS, maxCoords.t);
			glVertex3f(-2.*localShift, 0.0, 0.0);

			localShift = i*nrmTriangleSide;
			glTexCoord2f(rightTexBorder, 0.0);
			glVertex3f(-2.*localShift, -1.0, 0.0);			
		}
		else
		{			
			// /\/\/|\+/\+/\
			// \/\/\|/ \/ \/
			localShift = i * nrmTriangleSide;
			glTexCoord2f(halfOfS + localShift, 0.0);
			glVertex3f(2.*localShift, -1., 0.0);

			localShift = nrmHalfTriangleSide + i*nrmTriangleSide;
			glTexCoord2f(halfOfS + localShift, halfOfT);
			glVertex3f(2.*localShift, 0.0, 0.0);

			localShift = (i + 1)*nrmTriangleSide;
			glTexCoord2f(halfOfS + localShift, 0.0);
			glVertex3f(2.*localShift, -1., 0.0);			

			// /\+/\+/|\/\/\
			// \/ \/ \|/\/\/
			localShift = (i + 1)*nrmTriangleSide;
			glTexCoord2f(halfOfS - localShift, 0.0);
			glVertex3f(-2.*localShift, -1.0, 0.0);

			localShift = nrmHalfTriangleSide + i*nrmTriangleSide;
			glTexCoord2f(halfOfS - localShift, halfOfT);
			glVertex3f(-2.*localShift, 0.0, 0.0);

			localShift = i*nrmTriangleSide;
			glTexCoord2f(halfOfS - localShift, 0.0);
			glVertex3f(-2.*localShift, -1.0, 0.0);			
		}
	}
	glEnd();


	
	// Bottom central triangle
	glBindTexture(GL_TEXTURE_2D, TextureObject2.Handle);
	glBegin(GL_TRIANGLES);

	for (i = 0 ; i < triNum ; i ++)
	{
		if (this->pattern == TexPattern::All )
		{
			// /\/\/|\/+\/+\
			// \/\/\|/\ /\ /
			localShift = nrmHalfTriangleSide + i*nrmTriangleSide; 
			glTexCoord2f(leftTexBorder, maxCoords.t);
			glVertex3f( 2.*localShift, 0.0, 0.0);		

			localShift = nrmHalfTriangleSide + (i+1)*nrmTriangleSide;
			glTexCoord2f(rightTexBorder, maxCoords.t);
			glVertex3f(2.*localShift, 0.0, 0.0);

			localShift = (i+1)*nrmTriangleSide;
			glTexCoord2f(halfOfS, 0.0);
			glVertex3f(2.*localShift, -1.0, 0.0);

			// /+\/+\/|\/\/\
			// \ /\ /\|/\/\/			
			localShift = nrmHalfTriangleSide + (i+1)*nrmTriangleSide; 
			glTexCoord2f(leftTexBorder, maxCoords.t);
			glVertex3f(-2.*localShift , 0.0, 0.0);

			localShift = nrmHalfTriangleSide + i*nrmTriangleSide; 
			glTexCoord2f(rightTexBorder, maxCoords.t);
			glVertex3f( -2.*localShift, 0.0, 0.0);		

			localShift = (i+1)*nrmTriangleSide; 
			glTexCoord2f(halfOfS, 0.0);
			glVertex3f(-2.*localShift, -1.0, 0.0);			
		}
		else
		{
			// /\/\/|\/+\/+\
			// \/\/\|/\ /\ /			
			localShift = nrmHalfTriangleSide + i*nrmTriangleSide; 
			glTexCoord2f(halfOfS + localShift, halfOfT);
			glVertex3f( 2.*localShift, 0.0, 0.0);		

			localShift = nrmHalfTriangleSide + (i+1)*nrmTriangleSide;
			glTexCoord2f(halfOfS + localShift, halfOfT);
			glVertex3f(2.*localShift, 0.0, 0.0);

			localShift = (i+1)*nrmTriangleSide;
			glTexCoord2f(halfOfS + localShift , 0.0);
			glVertex3f(2.*localShift, -1.0, 0.0);					

			// /+\/+\/|\/\/\
			// \ /\ /\|/\/\/
			localShift = nrmHalfTriangleSide + (i+1)*nrmTriangleSide; 
			glTexCoord2f(halfOfS - localShift, halfOfT);
			glVertex3f(-2.*localShift , 0.0, 0.0);

			localShift = nrmHalfTriangleSide + i*nrmTriangleSide; 
			glTexCoord2f(halfOfS - localShift, halfOfT);
			glVertex3f( -2.*localShift, 0.0, 0.0);		

			localShift = (i+1)*nrmTriangleSide; 
			glTexCoord2f(halfOfS - localShift, 0.0);
			glVertex3f(-2.*localShift, -1.0, 0.0);
		}

		if ((i == 0 && this->pattern == TexPattern::WithAdjacents) || this->pattern == TexPattern::All)
		{
			// /\/\/|\ /\ /\
			// \/\/\|/+\/+\/
			localShift = i*nrmTriangleSide;
			glTexCoord2f(leftTexBorder, maxCoords.t);
			glVertex3f(2.*localShift, 1., 0.0);

			localShift = (i + 1)*nrmTriangleSide;
			glTexCoord2f(rightTexBorder, maxCoords.t);
			glVertex3f(2.*localShift, 1., 0.0);

			localShift = nrmHalfTriangleSide + i*nrmTriangleSide;
			glTexCoord2f(halfOfS, 0.0);
			glVertex3f(2.*localShift, 0.0, 0.0);
			

			// /\ /\ /|\/\/\
			// \/+\/+\|/\/\/			
			localShift = (i + 1)*nrmTriangleSide;
			glTexCoord2f(leftTexBorder, maxCoords.t);
			glVertex3f(-2.*localShift, 1.0, 0.0);

			localShift = i*nrmTriangleSide;
			glTexCoord2f(rightTexBorder, maxCoords.t);
			glVertex3f(-2.*localShift, 1.0, 0.0);

			localShift = nrmHalfTriangleSide + i*nrmTriangleSide;
			glTexCoord2f(halfOfS, 0.);
			glVertex3f(-2.*localShift, 0.0, 0.0);			
		}
		else
		{
			// /\/\/|\ /\ /\
			// \/\/\|/+\/+\/
			localShift = i*nrmTriangleSide;
			glTexCoord2f(halfOfS + localShift, maxCoords.t);
			glVertex3f(2.*localShift, 1., 0.0);

			localShift = (i + 1)*nrmTriangleSide;
			glTexCoord2f(halfOfS + localShift, maxCoords.t);
			glVertex3f(2.*localShift, 1., 0.0);

			localShift = nrmHalfTriangleSide + i*nrmTriangleSide;
			glTexCoord2f(halfOfS + localShift, halfOfT);
			glVertex3f(2.*localShift, 0.0, 0.0);

			// /\ /\ /|\/\/\
			// \/+\/+\|/\/\/			
			localShift = (i + 1)*nrmTriangleSide;
			glTexCoord2f(halfOfS - localShift, maxCoords.t);
			glVertex3f(-2.*localShift, 1.0, 0.0);

			localShift = i*nrmTriangleSide;
			glTexCoord2f(halfOfS - localShift, maxCoords.t);
			glVertex3f(-2.*localShift, 1.0, 0.0);

			localShift = nrmHalfTriangleSide + i*nrmTriangleSide;
			glTexCoord2f(halfOfS - localShift, halfOfT);
			glVertex3f(-2.*localShift, 0.0, 0.0);
		}
	}

	glEnd();	
	

	return FF_SUCCESS;
}


static const std::string vertexShaderCode = STRINGIFY(
	void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_FrontColor = gl_Color;
}
);


static const std::string fragmentShaderCode = STRINGIFY(
	uniform sampler2D inputTexture;
void main()
{
	vec4 color = texture2D(inputTexture, gl_TexCoord[0].st);	
	gl_FragColor = color;
}
);


FFResult	FFGLTriangleMixer::InitGL(const FFGLViewportStruct *vp)
{
	//initialize gl shader
	m_shader.Compile(vertexShaderCode, fragmentShaderCode);

	//activate our shader
	m_shader.BindShader();

	//to assign values to parameters in the shader, we have to lookup
	//the "location" of each value.. then call one of the glUniform* methods
	//to assign a value
	auto m_inputTextureLocation = m_shader.FindUniform("inputTexture");	

	//the 0 means that the 'inputTexture' in
	//the shader will use the texture bound to GL texture unit 0
	glUniform1i(m_inputTextureLocation, 0);

	m_shader.UnbindShader();

	return FF_SUCCESS;
}

FFResult	FFGLTriangleMixer::DeInitGL()
{
	return FF_SUCCESS;
}

