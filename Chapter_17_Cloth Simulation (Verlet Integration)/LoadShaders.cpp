#include "LoadShaders.h"
#include "OGL.h"

#define CHAR_BUFFER_SIZE 1024


//ReadShaderFromFile
static bool ReadShaderFromFile( /* IN */ const char *fileName, /* OUT (should be NULL)*/ char **returnSource)
{
	//code
	if (fileName == nullptr)
		return(false);

	FILE *fp = nullptr;

#ifdef _WIN32
	fopen_s(&fp, fileName, "r");
#else
	fp = fopen(fileName, "r");
#endif

	if (fp == nullptr)
	{
		*returnSource = (char *) calloc( 1, CHAR_BUFFER_SIZE );
		if( returnSource)
		{
			sprintf( *returnSource, "Error While Opening File '%s'\n", fileName);
		}
		
		return( false);
	}

	fseek(fp, 0, SEEK_END);
	int length = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	*returnSource = (char *) calloc(1, length + 1);
	fread( *returnSource, length, 1, fp);
	*(*returnSource + length) = '\0';
	fclose(fp);
	fp = nullptr;

	return( true);
}


//GetShaderType()
char* GetShaderType( GLenum type)
{
	switch( type)
	{
		case GL_VERTEX_SHADER:
			return "GL_VERTEX_SHADER";
		break;

		case GL_TESS_CONTROL_SHADER:
			return "GL_TESS_CONTROL_SHADER";
		break;

		case GL_TESS_EVALUATION_SHADER:
			return "GL_TESS_EVALUATION_SHADER";
		break;

		case GL_GEOMETRY_SHADER:
			return "GL_GEOMETRY_SHADER";
		break;

		case GL_FRAGMENT_SHADER:
			return "GL_FRAGMENT_SHADER";
		break;

		default:
			return "UNKNOWN_SHADER";
		break;
	}
}


//CreateProgram
GLuint CreateProgram(SHADERS_INFO *shaderInfo, int shaderCount, BIND_ATTRIBUTES_INFO *attribInfo, int attribCount, FEEDBACK_INFO *feedBack)
{
	//variables
	char *source = nullptr;
	GLuint programID;

	GLint infoLogLength = 0;
	GLint shaderCompiledStatus, programLinkStatus;
	char *szInfoLog = NULL;

	//code
	programID = glCreateProgram();

	for(int i = 0; i < shaderCount; i++)
	{
		if ((shaderInfo + i)->shaderLoadAs == SHADER_INFO_LOAD_FROM_FILE)
		{
			if (ReadShaderFromFile((shaderInfo + i)->shaderFileName, &source) == false)
			{
				Log( "LoadShader Error: error in reading file %s.\n", source);

				if(source)
				{
					free(source);
					source = nullptr;
				}

				for( int j = 0; j < i; j++)
				{
					glDetachShader(programID, (shaderInfo + j)->shaderID);
					glDeleteShader(( shaderInfo + j)->shaderID);
					( shaderInfo + j)->shaderID = 0;
				}

				glDeleteProgram(programID);
				return(0);
			}
		}
		else
		{
			source = (char *) ( ( shaderInfo + i)->shaderSource);
		}


		( shaderInfo + i)->shaderID = glCreateShader(( shaderInfo + i)->shaderType);
		if (( shaderInfo + i)->shaderID == 0)
		{
			Log( "LoadShader Error: glCreateShader() failed for %s.\n", GetShaderType(( shaderInfo + i)->shaderType));

			for( int j = 0; j < i; j++)
			{
				glDetachShader(programID, (shaderInfo + j)->shaderID);
				glDeleteShader(( shaderInfo + j)->shaderID);
				( shaderInfo + j)->shaderID = 0;
			}

			if ((shaderInfo + i)->shaderLoadAs == SHADER_INFO_LOAD_FROM_FILE)
			{
				free((void*)source);
				source = NULL;
			}

			glDeleteProgram(programID);
			return(0);
		}

		//feed source code to shader object
		glShaderSource( ( shaderInfo + i)->shaderID, 1, (const GLchar**) &source, NULL);

		//Compile Source code
		glCompileShader( ( shaderInfo + i)->shaderID);

		//Compilation status
		glGetShaderiv( ( shaderInfo + i)->shaderID, GL_COMPILE_STATUS, &shaderCompiledStatus);
		if( shaderCompiledStatus == GL_FALSE)   //Compilation Failed
		{
			//get shader compile log length
			glGetShaderiv( ( shaderInfo + i)->shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
			if(infoLogLength > 0)
			{
				//allocate memory for log
				szInfoLog = (char *) malloc( sizeof(char) * infoLogLength);
				if(szInfoLog != NULL)
				{
					//get log
					glGetShaderInfoLog( ( shaderInfo + i)->shaderID, infoLogLength, NULL, szInfoLog);

					Log( "LoadShader Error: %s Compile Log : %s\n", GetShaderType(( shaderInfo + i)->shaderType), szInfoLog);

					//free memory
					free(szInfoLog);

					for( int j = 0; j < i; j++)
					{
						glDetachShader(programID, (shaderInfo + j)->shaderID);
						glDeleteShader(( shaderInfo + j)->shaderID);
						( shaderInfo + j)->shaderID = 0;
					}

					if ((shaderInfo + i)->shaderLoadAs == SHADER_INFO_LOAD_FROM_FILE)
					{
						free((void*)source);
						source = NULL;
					}

					glDeleteProgram(programID);
					return(0);
				}
			}
		}

		if ((shaderInfo + i)->shaderLoadAs == SHADER_INFO_LOAD_FROM_FILE)
		{
			free((void*)source);
			source = NULL;
		}

		glAttachShader(programID, (shaderInfo + i)->shaderID);
	}

	//bind attributes
	if( attribInfo)
	{
		for (int i = 0; i < attribCount; i++)
			glBindAttribLocation(programID, (attribInfo + i)->index, (attribInfo + i)->attribute);
	}

	//transform feedback
	if( feedBack)
	{
		glTransformFeedbackVaryings( programID, feedBack->varyingCount, feedBack->varying, feedBack->bufferMode);
	}

	//Link Program
	glLinkProgram(programID);

	//linking status
	glGetProgramiv(programID, GL_LINK_STATUS, &programLinkStatus);
	if (programLinkStatus == GL_FALSE)   //Linking Failed
	{
		//get link log length
		glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
		if (infoLogLength > 0)
		{
			//allocate memory for log
			szInfoLog = (char*)malloc(infoLogLength);
			if (szInfoLog != NULL)
			{
				glGetProgramInfoLog(programID, infoLogLength, NULL, szInfoLog);

				Log( "LoadShader Error: Shader Program Linking Log : %s\n", szInfoLog);

				//free memory
				free(szInfoLog);

				for( int j = 0; j < shaderCount; j++)
				{
					glDetachShader(programID, (shaderInfo + j)->shaderID);
					glDeleteShader(( shaderInfo + j)->shaderID);
					( shaderInfo + j)->shaderID = 0;
				}

				glDeleteProgram(programID);
				return(0);
			}
		}
	}

	return(programID);
}



//DeleteProgram()
void DeleteProgram(GLuint program)
{
	//varriables
	GLsizei shaderCount;
	GLsizei actualShaderCount;

	//code
	glUseProgram(program);

	glGetProgramiv(program, GL_ATTACHED_SHADERS, &shaderCount);
	GLuint *pShader = (GLuint *)malloc(shaderCount * sizeof(GLuint));

	glGetAttachedShaders(program, shaderCount, &actualShaderCount, pShader);
	
	for (int i = 0; i < shaderCount; i++)
	{
		glDetachShader(program, pShader[i]);
		glDeleteShader(pShader[i]);
	}

	glDeleteProgram(program);
	program = 0;

	glUseProgram(0);

	free(pShader);
	pShader = 0;
}


#undef CHAR_BUFFER_SIZE

