#pragma once

#include "FFGL.h"
#include <string>

class FFGLParameter
{
private:

	const float TrueValue{ 1.0f };
	const float FalseValue{ 0.0f };

	std::string _paramName;
	DWORD		_paramType;
	bool		isChanged;

	std::string _strValueStorage{ "" };	
	float	m_floatValueStorage{ 0 };	

public:

	FFGLParameter(const std::string& paramName, DWORD paramType);

	// 
	FFGLParameter(const std::string& paramName, const std::string& strValue);

	// Float value parameter (FF_TYPE_STANDARD)
	FFGLParameter(const std::string& paramName, float floatValue);

	// Boolean value parameter (FF_TYPE_BOOLEAN)
	FFGLParameter(const std::string & paramName, bool boolValue);

	~FFGLParameter();

	DWORD	getType()
	{
		return _paramType;
	}

	const std::string& getName()
	{
		return _paramName;
	}

	float getFloatStorage()
	{
		return m_floatValueStorage;
	}

	operator bool()
	{
		return m_floatValueStorage > 0.0;
	}

	operator float()
	{
		return m_floatValueStorage;
	}

	const std::string&	getStrStorage()
	{
		return _strValueStorage;
	}


	void setValue(float newValue)
	{
		if (m_floatValueStorage != newValue)
		{
			m_floatValueStorage = newValue;
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

