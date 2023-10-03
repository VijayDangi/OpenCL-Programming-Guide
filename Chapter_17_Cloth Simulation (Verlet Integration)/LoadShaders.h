#ifndef __VJD_LOAD_SHADER_H__
#define __VJD_LOAD_SHADER_H__

#include <Windows.h>
#include <stdio.h>

#include <GL/glew.h>    //Graphic Library Extension Wrangler
#include <gl/gl.h>

#define SHADER_INFO_LOAD_FROM_FILE	0x1
#define SHADER_INFO_LOAD_FROM_STRING  0x2

typedef struct BIND_ATTRIBUTES_INFO
{
	const char *attribute;
	GLuint index;

} BIND_ATTRIBUTES_INFO;

typedef struct SHADERS_INFO
{
	GLenum shaderType;
	GLenum shaderLoadAs;
	union
	{
		const char *shaderSource;
		const char *shaderFileName;
	};

	GLuint shaderID;

}SHADERS_INFO;

typedef struct FEEDBACK_INFO
{
	const char **varying;
	int varyingCount;
	GLenum bufferMode;
} FEEDBACK_INFO;


//function declaration
GLuint CreateProgram(SHADERS_INFO *shaderInfo, int shaderCount, BIND_ATTRIBUTES_INFO *attribInfo, int attribCount, FEEDBACK_INFO *feedBack);

void DeleteProgram(GLuint program);

#endif
