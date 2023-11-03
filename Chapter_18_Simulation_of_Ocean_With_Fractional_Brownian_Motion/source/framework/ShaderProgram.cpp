
#include "../Common.h"
#include "ShaderProgram.h"
#include "../system/FileIO.h"

/**
 * @brief GetShaderTypeString()
 * @param type 
 * @return 
 */
static char* GetShaderTypeString( GLenum type)
{
    // code
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

		case GL_COMPUTE_SHADER:
			return "GL_COMPUTE_SHADER";
		break;

		default:
			return "UNKNOWN_SHADER";
		break;
	}
}

/**
 * @brief IsValidShaderType()
 * @param shader_type 
 * @return 
 */
static bool IsValidShaderType( GLenum shader_type)
{
    // code
    switch( shader_type)
	{
		case GL_VERTEX_SHADER:
		case GL_TESS_CONTROL_SHADER:
		case GL_TESS_EVALUATION_SHADER:
		case GL_GEOMETRY_SHADER:
		case GL_FRAGMENT_SHADER:
		case GL_COMPUTE_SHADER:
            return true;

		default:
            return false;
	}
}

/**
 * @brief ShaderProgram()
 */
ShaderProgram::ShaderProgram()
{
    // code
    m_program_id = glCreateProgram();
}

/**
 * @brief ~ShaderProgram()
 */
ShaderProgram::~ShaderProgram()
{
    // code
    Release();
    m_uniforms.clear();
    m_attributes.clear();
}

/**
 * @brief CompileShader()
 * @param shader_source 
 * @param shader_type 
 * @return 
 */
GLuint ShaderProgram::CompileShader( const char *shader_source, GLenum shader_type)
{
    // variable declaration
    GLuint shader_id = 0;

    // code
    if( !IsValidShaderType(shader_type))
    {
        Log("Error: Invalid Shader type.");
        return 0;
    }

    shader_id = glCreateShader( shader_type);

        // feed source code to shader object
    glShaderSource( shader_id, 1, (const char**)&shader_source, nullptr);

        // compile shader code
    glCompileShader( shader_id);

        // compile status
    GLint shader_compile_status;

    glGetShaderiv( shader_id, GL_COMPILE_STATUS, &shader_compile_status);
    if( shader_compile_status == GL_FALSE)
    {
        GLint log_length;

            // get shader compile log length
        glGetShaderiv( shader_id, GL_INFO_LOG_LENGTH, &log_length);
        if( log_length > 0)
        {
            // allocate memory for log
            char *info_log = (char *) calloc( log_length, sizeof( char));
            if( info_log)
            {
                // get log
                glGetShaderInfoLog( shader_id, log_length, nullptr, info_log);

                Log( "Error: %s Compile log: %s\n", GetShaderTypeString( shader_type), info_log);

                free( info_log);
            }
        }

        glDeleteShader( shader_id);
        return 0;
    }

    return shader_id;
}

/**
 * @brief AddShaderFromFile()
 * @param shader_source_file 
 * @param shader_type 
 * @return 
 */
bool ShaderProgram::AddShaderFromFile( const char *shader_source_file, GLenum shader_type)
{
    // variable declaration
    std::string shader_source;
    GLuint shader_id = 0;

    // code
    if( !IsValidShaderType(shader_type))
    {
        Log("Error: Invalid Shader type.");
        return false;
    }

    if( FileIO::ReadFile( shader_source_file, shader_source) == false)
    {
        Log("Error: File \"%s\" unable to read.", shader_source_file);
        return false;
    }

    if( (shader_id = CompileShader( shader_source.c_str(), shader_type)) == 0)
    {
        Log("Error: shader compilation failed for \"%s\".", shader_source_file);
        shader_source.clear();
        return false;
    }

    glAttachShader( m_program_id, shader_id);

    shader_source.clear();

    return true;
}

/**
 * @brief AddShaderFromSource()
 * @param shader_source_string 
 * @param shader_type 
 * @return 
 */
bool ShaderProgram::AddShaderFromSource( const char *shader_source_string, GLenum shader_type)
{
    // variable declaration
    GLuint shader_id = 0;

    // code
    if( !IsValidShaderType(shader_type))
    {
        Log("Error: Invalid Shader type.");
        return false;
    }

    if( !shader_source_string)
    {
        Log("Error: Null parameter.");
        return false;
    }

    if( (shader_id = CompileShader( shader_source_string, shader_type)) == 0)
    {
        Log("Error: failed to compile shader.");
        return false;
    }

    glAttachShader( m_program_id, shader_id);

    return true;
}

/**
 * @brief AddShaderID()
 * @param shader_id 
 * @param shader_type 
 * @return 
 */
bool ShaderProgram::AddShaderID( GLuint shader_id, GLenum shader_type)
{
    // code
    if( !IsValidShaderType(shader_type))
    {
        Log("Error: Invalid Shader type.");
        return false;
    }

    if( glIsShader( shader_id) == GL_FALSE)
    {
        Log("Error: Not a shader object");
        return false;
    }

    GLint ret_shader_type;
    glGetShaderiv( shader_id, GL_SHADER_TYPE, &ret_shader_type);
    if( shader_type != ret_shader_type)
    {
        Log("Error: Shader type not matched with parameter type");
        return false;
    }

    glAttachShader( m_program_id, shader_id);

    return true;
}

/**
 * @brief BindVertexAttributeLocation()
 * @param attribute 
 * @param index 
 */
void ShaderProgram::BindVertexAttributeLocation( const char *attribute, GLuint index)
{
    // code
    if( !attribute)
    {
        Log("Error: Null Parameter.");
        return;
    }

    glBindAttribLocation( m_program_id, index, attribute);
}

static char *GetType( GLenum type)
{
    switch( type)
    {
        case GL_FLOAT:                  return TO_STRING(GL_FLOAT);
        case GL_FLOAT_VEC2:             return TO_STRING(GL_FLOAT_VEC2);
        case GL_FLOAT_VEC3:             return TO_STRING(GL_FLOAT_VEC3);
        case GL_FLOAT_VEC4:             return TO_STRING(GL_FLOAT_VEC4);
        case GL_FLOAT_MAT2:             return TO_STRING(GL_FLOAT_MAT2);
        case GL_FLOAT_MAT3:             return TO_STRING(GL_FLOAT_MAT3);
        case GL_FLOAT_MAT4:             return TO_STRING(GL_FLOAT_MAT4);
        case GL_FLOAT_MAT2x3:           return TO_STRING(GL_FLOAT_MAT2x3);
        case GL_FLOAT_MAT2x4:           return TO_STRING(GL_FLOAT_MAT2x4);
        case GL_FLOAT_MAT3x2:           return TO_STRING(GL_FLOAT_MAT3x2);
        case GL_FLOAT_MAT3x4:           return TO_STRING(GL_FLOAT_MAT3x4);
        case GL_FLOAT_MAT4x2:           return TO_STRING(GL_FLOAT_MAT4x2);
        case GL_FLOAT_MAT4x3:           return TO_STRING(GL_FLOAT_MAT4x3);
        case GL_INT:                    return TO_STRING(GL_INT);
        case GL_INT_VEC2:               return TO_STRING(GL_INT_VEC2);
        case GL_INT_VEC3:               return TO_STRING(GL_INT_VEC3);
        case GL_INT_VEC4:               return TO_STRING(GL_INT_VEC4);
        case GL_UNSIGNED_INT:           return TO_STRING(GL_UNSIGNED_INT);
        case GL_UNSIGNED_INT_VEC2:      return TO_STRING(GL_UNSIGNED_INT_VEC2);
        case GL_UNSIGNED_INT_VEC3:      return TO_STRING(GL_UNSIGNED_INT_VEC3);
        case GL_UNSIGNED_INT_VEC4:      return TO_STRING(GL_UNSIGNED_INT_VEC4);
        case GL_DOUBLE:                 return TO_STRING(GL_DOUBLE);
        case GL_DOUBLE_VEC2:            return TO_STRING(GL_DOUBLE_VEC2);
        case GL_DOUBLE_VEC3:            return TO_STRING(GL_DOUBLE_VEC3);
        case GL_DOUBLE_VEC4:            return TO_STRING(GL_DOUBLE_VEC4);
        case GL_DOUBLE_MAT2:            return TO_STRING(GL_DOUBLE_MAT2);
        case GL_DOUBLE_MAT3:            return TO_STRING(GL_DOUBLE_MAT3);
        case GL_DOUBLE_MAT4:            return TO_STRING(GL_DOUBLE_MAT4);
        case GL_DOUBLE_MAT2x3:          return TO_STRING(GL_DOUBLE_MAT2x3);
        case GL_DOUBLE_MAT2x4:          return TO_STRING(GL_DOUBLE_MAT2x4);
        case GL_DOUBLE_MAT3x2:          return TO_STRING(GL_DOUBLE_MAT3x2);
        case GL_DOUBLE_MAT3x4:          return TO_STRING(GL_DOUBLE_MAT3x4);
        case GL_DOUBLE_MAT4x2:          return TO_STRING(GL_DOUBLE_MAT4x2);
        case GL_DOUBLE_MAT4x3:          return TO_STRING(GL_DOUBLE_MAT4x3);
        default:                        return "UNKNOWN";
    }
}

/**
 * @brief Build()
 * @return 
 */
bool ShaderProgram::Build()
{
    // code
        // link program
    glLinkProgram( m_program_id);

        // linking status
    GLint program_link_status;

    glGetProgramiv( m_program_id, GL_LINK_STATUS, &program_link_status);
    if( program_link_status == GL_FALSE)
    {
        // info log length
        GLint log_length = 0;

        glGetProgramiv( m_program_id, GL_INFO_LOG_LENGTH, &log_length);
        if( log_length > 0)
        {
            char *info_log = new char[log_length];
            if( info_log)
            {
                glGetProgramInfoLog( m_program_id, log_length, nullptr, info_log);
                Log( "Shader Program Link Error log: %s.", info_log);

                delete info_log;
                info_log = nullptr;
            }
        }

        return false;
    }

    int count = -1;
    int length = 0;
    char name[256];
    char testName[256];
    int size;
    GLenum type;

    glUseProgram( m_program_id);

        // Populate Attributes
        glGetProgramiv( m_program_id, GL_ACTIVE_ATTRIBUTES, &count);

        for( int i = 0; i < count; ++i)
        {
            memset( name, 0, sizeof( char) * 256);
            glGetActiveAttrib( m_program_id, (GLuint)i, 256, &length, &size, &type, name);
            
            int attrib_location = glGetAttribLocation( m_program_id, name);
            if( attrib_location >= 0)
            {
                Log( "Attrib Name: \"%s\", type: \"%s\", index: \"%d\"", name, GetType(type), attrib_location);
                m_attributes[name] = attrib_location;
            }
        }

        // populate Uniform
        count = -1;
        length = 0;

        glGetProgramiv( m_program_id, GL_ACTIVE_UNIFORMS, &count);

        for( int i = 0; i < count; ++i)
        {
            memset( name, 0, sizeof( char) * 256);
            glGetActiveUniform( m_program_id, (GLuint)i, 256, &length, &size, &type, name);
            int uniform_location = glGetUniformLocation( m_program_id, name);

            Log( "Uniform Name: \"%s\", type: \"%s\", index: \"%d\"", name, GetType(type), uniform_location);
            if( uniform_location >= 0)  // Is Valid Uniform
            {
                std::string uniformName = name;

                // if contains [, uniform is array
                std::size_t found = uniformName.find( '[');
                if( found != std::string::npos)
                {
                    uniformName.erase( uniformName.begin() + found, uniformName.end());
                    
                    int uniformIndex = 0;
                    while( true)
                    {
                        memset( testName, 0, sizeof(char) * 256);
                        sprintf( testName, "%s[%d]", uniformName.c_str(), uniformIndex++);
                        int uniformLocation = glGetUniformLocation( m_program_id, testName);
                        if( uniformLocation < 0)
                        {
                            break;
                        }
                        m_uniforms[testName] = uniformLocation;
                    }
                }
                m_uniforms[uniformName] = uniform_location;
            }
        }


    glUseProgram( 0);

    return true;
}

/**
 * @brief GetUniformLocation()
 * @param  
 * @return 
 */
GLint ShaderProgram::GetUniformLocation( const char *name)
{
    // code
    std::map<std::string, GLint>::iterator itr = m_uniforms.find( name);
    if( itr == m_uniforms.end())
    {
        Log( "Invalid Uniform \"%s\"", name);
        return -1;
    }

    return itr->second;
}

/**
 * @brief GetAttributeLocation()
 * @param  
 * @return 
 */
GLuint ShaderProgram::GetAttributeLocation( const char *name)
{
    // code
    std::map<std::string, GLuint>::iterator itr = m_attributes.find( name);
    if( itr == m_attributes.end())
    {
        Log( "Invalid Attribute \"%s\"", name);
        return -1;
    }

    return itr->second;
}

/**
 * @brief GetID()
 * @return 
 */
GLuint ShaderProgram::GetID()
{
    return m_program_id;
}

/**
 * @brief Bind()
 */
void ShaderProgram::Bind()
{
    // code
    glUseProgram( m_program_id);
}

/**
 * @brief Unbind()
 */
void ShaderProgram::Unbind()
{
    // code
    glUseProgram( 0);
}

/**
 * @brief Release()
 */
void ShaderProgram::Release()
{
    // variable declaration
    GLsizei shader_count;
    GLsizei actual_shader_count;

    // code
    if( m_program_id)
    {
        glGetProgramiv( m_program_id, GL_ATTACHED_SHADERS, &shader_count);
        GLuint *p_shader = new GLuint[shader_count];

        glGetAttachedShaders( m_program_id, shader_count, &actual_shader_count, p_shader);

        for( int i = 0; i < shader_count; ++i)
        {
            glDetachShader( m_program_id, p_shader[i]);
            glDeleteShader( p_shader[i]);
        }

        glDeleteProgram( m_program_id);
        m_program_id = 0;

        delete p_shader;
        p_shader = nullptr;
    }
}

