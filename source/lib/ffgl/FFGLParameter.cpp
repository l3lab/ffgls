#include "FFGLParameter.h"

FFGLParameter::FFGLParameter(const std::string& paramName, DWORD paramType)
{
	_paramName.assign(paramName);
	_paramType = paramType;
}

FFGLParameter::FFGLParameter(const std::string& paramName, const std::string& strValue) : FFGLParameter(paramName, (DWORD)FF_TYPE_TEXT)
{
	_strValueStorage.assign(strValue);
}

FFGLParameter::FFGLParameter(const std::string& paramName, float floatValue) : FFGLParameter(paramName, (DWORD)FF_TYPE_STANDARD)
{
	_floatValueStorage = floatValue;
}

FFGLParameter::~FFGLParameter()
{
	_paramName.clear();
}
