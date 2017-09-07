#version 420 core

layout(location=0) in vec4 terrainCoords;
layout(location=1) in vec4 terrainColors;

uniform mat4 projMat;
uniform mat4 modelViewMat;

flat out vec4 colorsExport;

uniform vec4 globAmb;
struct Material
{
	vec4 ambRefl;
	vec4 difRefl;
	vec4 specRefl;
	vec4 emitCols;
	float shininess;
};

uniform Material terrainFandB;

void main(void)
{
   gl_Position = projMat * modelViewMat * terrainCoords;
   colorsExport = globAmb * terrainFandB.ambRefl;
}


