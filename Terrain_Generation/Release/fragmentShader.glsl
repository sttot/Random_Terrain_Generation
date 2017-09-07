#version 420 core

flat in vec4 colorsExport;

out vec4 colorsOut;

void main(void)
{
   colorsOut = colorsExport;
}