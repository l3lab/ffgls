#pragma once

#include <string>
#include <math.h>
#include <map>
#include <memory>
#include <vector>


#define STRINGIFY( expr ) #expr

using namespace std;


const string vertexMulticoordShaderCode{ STRINGIFY(
	void main()
	{
		gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
		gl_TexCoord[0] = gl_MultiTexCoord0;
		gl_TexCoord[1] = gl_MultiTexCoord1;
		gl_TexCoord[2] = gl_MultiTexCoord2;
		gl_FrontColor = gl_Color;
	})
};

// Pixel shader: uniform variables
const string UNIFORMS{ STRINGIFY(
	//
	uniform sampler2D texture0;
	uniform sampler2D texture1;
	uniform sampler2D texture2;

	uniform float dx;
	uniform float dy;

	uniform float alpha;
	uniform float imgFactor;
	uniform float velocity;
	uniform float velocityScale;

	uniform float noiseScale;

	uniform float x1;
	uniform float y1;

	uniform float x2;
	uniform float y2;

	uniform float x3;
	uniform float y3;
) };

const string getFieldFuncStart{STRINGIFY(	
	vec2 getField()
	{
		vec2 coords = 2 * (gl_TexCoord[2].st - 0.5);
		float x = coords.x;
		float y = coords.y;

		// return expression
) };

const string getFieldFuncEnd{ STRINGIFY(
	; }


) };

// 3x3 sobel operator
const string SOBEL_FIELD{ STRINGIFY(
	//
	float getWeight(vec4 cVal)
	{
		return cVal.r + cVal.g + cVal.b + cVal.a;
	}
	//
	vec2 getField()
	{
		vec2 gradient = vec2(0.0);
		vec2 texCoords = gl_TexCoord[2].xy;

		float z1 = getWeight(texture2D(texture2, vec2(texCoords.x - dx, texCoords.y + dy)));
		float z2 = getWeight(texture2D(texture2, vec2(texCoords.x, texCoords.y + dy)));
		float z3 = getWeight(texture2D(texture2, vec2(texCoords.x + dx, texCoords.y + dy)));
		float z4 = getWeight(texture2D(texture2, vec2(texCoords.x - dx, texCoords.y)));

		float z6 = getWeight(texture2D(texture2, vec2(texCoords.x + dx, texCoords.y)));
		float z7 = getWeight(texture2D(texture2, vec2(texCoords.x - dx, texCoords.y - dy)));
		float z8 = getWeight(texture2D(texture2, vec2(texCoords.x, texCoords.y - dy)));
		float z9 = getWeight(texture2D(texture2, vec2(texCoords.x + dx, texCoords.y - dy)));

		gradient.y = (z7 + 2 * z8 + z9) - (z1 + 2 * z2 + z3);
		gradient.x = -(z3 + 2 * z6 + z9) + (z1 + 2 * z4 + z7);

		return gradient;
	}
) };


// Get field direction from the image itself (through the colour-to-value mapping)
const string IMAGE_FIELD{ STRINGIFY(
	
	//
	vec2  getField()
	{
		vec2 field = vec2(0.0);
		vec2 texCoords = gl_TexCoord[2].xy;

		vec4 texValue = texture2D(texture2, texCoords);
		field = texValue.xy - vec2(0.5);
		return field * texValue.z;
	}
)};

// Advection operator
const string ADVECTION{ STRINGIFY(
	
	//	
	void main()
	{
		vec2 texCoord = gl_TexCoord[0].st;
		vec2 coords = vec2(-1.0, -1.0) + 2.0 * texCoord;
		vec4 imgColor = texture2D(texture2, gl_TexCoord[2]);

		vec2 field = velocityScale * (velocity - 0.5) * getField();

		vec2 samplerCoord = texCoord - field;
		vec4 srcColor = texture2D(texture0, samplerCoord);
		vec4 noiseColor = texture2D(texture1, 0.5*vec2(1 - noiseScale) + noiseScale * texCoord);
		gl_FragColor = (1 - alpha) * (((1 - imgFactor) * srcColor) + (imgFactor * imgColor)) + alpha * noiseColor;
	}
)};

const string defaultVertexShaderCode{ STRINGIFY(
	void main()
	{
		gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
		gl_TexCoord[0] = gl_MultiTexCoord0;
		gl_FrontColor = gl_Color;
	}
)};


const string defaultFragmentShaderCode{ STRINGIFY(
	uniform sampler2D inputTexture;
	void main()
	{
		vec4 color = vec4(1.0, 0.0, 0.0, 1.0);
		color = texture2D(inputTexture, gl_TexCoord[0].st);
		gl_FragColor = color;
	}
)};

const string defaultCalulations{ STRINGIFY(return vec2(0.0, 0.0)) };
