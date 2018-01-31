#pragma once

#include "FFGL.h"
#include <string>

class FFGLParameter
{
private:

	std::string _paramName;
	DWORD		_paramType;
	bool		isChanged;

	std::string _strValueStorage{ "" };
	float	_floatValueStorage{ 0 };

public:

	FFGLParameter(const std::string& paramName, DWORD paramType);
	FFGLParameter(const std::string& paramName, const std::string& strValue);
	FFGLParameter(const std::string& paramName, float floatValue);
	~FFGLParameter();

	DWORD	getType()
	{
		return _paramType;
	}

	std::string& getName()
	{
		return _paramName;
	}

	float getFloatStorage()
	{
		return _floatValueStorage;
	}

	std::string&	getStrStorage()
	{
		return _strValueStorage;
	}


	void setFloatStorage(float newValue)
	{
		if (_floatValueStorage != newValue)
		{
			_floatValueStorage = newValue;
			isChanged = true;
		}
		else
		{
			isChanged = false;
		}
	}

	void setStrStorage(const char* newValue)
	{
		if (_strValueStorage.compare(newValue) != 0)
		{
			_strValueStorage.assign(newValue);
			isChanged = true;
		}
		else
		{
			isChanged = false;
		}
	}

	bool getChanged()
	{
		return isChanged;
	}


	

};

