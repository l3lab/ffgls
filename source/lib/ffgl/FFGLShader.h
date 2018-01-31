#ifndef FFGLShader_H
#define FFGLShader_H

#include <FFGL.h>
#include <string>
#include <map>
#include <memory>
#include <vector>


class FFGLShader
{
public:
	FFGLShader();
	virtual ~FFGLShader();

	int IsReady();
	int Compile(const std::string& vtxProgram, const std::string& fragProgram);
	int Compile(const std::string& vtxProgram, const std::string& fragProgram, const std::map<std::string, int>& attributeValues);
	

	GLuint FindUniform(const char *name);
	GLint FindAttribute(const GLchar * name);

	int BindShader();
	int UnbindShader();
	void FreeGLResources();

private:
	GLenum m_glProgram;
	GLenum m_glVertexShader;
	GLenum m_glFragmentShader;
	GLuint m_linkStatus;
	void CreateGLResources();

	int Compile(const char *vtxProgram, const char *fragProgram);
	int Compile(const char *vtxProgram, const char *fragProgram, const std::map<std::string, int>* attributeValues);
};

#endif
