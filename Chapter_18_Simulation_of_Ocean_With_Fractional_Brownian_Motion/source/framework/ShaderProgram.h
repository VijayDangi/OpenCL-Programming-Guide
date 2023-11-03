#pragma once

#include <map>
#include <vector>
#include <string>

class ShaderProgram
{
    private:
        GLuint m_program_id;
        std::map<std::string, GLint> m_uniforms;
        std::map<std::string, GLuint> m_attributes;

    public:
        ShaderProgram();
        ~ShaderProgram();

        bool AddShaderFromFile( const char *shader_source_file, GLenum shader_type);
        bool AddShaderFromSource( const char *shader_source_string, GLenum shader_type);
        bool AddShaderID( GLuint shader_id, GLenum shader_type);
        void BindVertexAttributeLocation( const char *attribute, GLuint index);
        bool Build();

        GLint GetUniformLocation( const char *name);
        GLuint GetAttributeLocation( const char *name);

        GLuint GetID();

        void Bind();
        void Unbind();

        void Release();

        // Log("%s => %d", uniform_name, glGetUniformLocation( m_program_id, uniform_name)); 

        inline void SetUniformInt( const char *uniform_name, GLint value) { glUniform1i( GetUniformLocation(uniform_name), value); }
        inline void SetUniformInt2( const char *uniform_name, GLint value0, GLint value1) { glUniform2i( GetUniformLocation(uniform_name), value0, value1); }
        inline void SetUniformInt3( const char *uniform_name, GLint value0, GLint value1, GLint value2) { glUniform3i( GetUniformLocation(uniform_name), value0, value1, value2); }
        inline void SetUniformInt4( const char *uniform_name, GLint value0, GLint value1, GLint value2, GLint value3) { glUniform4i( GetUniformLocation(uniform_name), value0, value1, value2, value3); }

        inline void SetUniformUInt( const char *uniform_name, GLuint value) { glUniform1ui( GetUniformLocation(uniform_name), value); }
        inline void SetUniformUInt2( const char *uniform_name, GLuint value0, GLuint value1) { glUniform2ui( GetUniformLocation(uniform_name), value0, value1); }
        inline void SetUniformUInt3( const char *uniform_name, GLuint value0, GLuint value1, GLuint value2) { glUniform3ui( GetUniformLocation(uniform_name), value0, value1, value2); }
        inline void SetUniformUInt4( const char *uniform_name, GLuint value0, GLuint value1, GLuint value2, GLuint value3) { glUniform4ui( GetUniformLocation(uniform_name), value0, value1, value2, value3); }

        inline void SetUniformFloat( const char *uniform_name, GLfloat value) { glUniform1f( GetUniformLocation(uniform_name), value); }
        inline void SetUniformFloat2( const char *uniform_name, GLfloat value0, GLfloat value1) { glUniform2f( GetUniformLocation(uniform_name), value0, value1); }
        inline void SetUniformFloat3( const char *uniform_name, GLfloat value0, GLfloat value1, GLfloat value2) { glUniform3f( GetUniformLocation(uniform_name), value0, value1, value2); }
        inline void SetUniformFloat4( const char *uniform_name, GLfloat value0, GLfloat value1, GLfloat value2, GLfloat value3) { glUniform4f( GetUniformLocation(uniform_name), value0, value1, value2, value3); }

        inline void SetUniformDouble( const char *uniform_name, GLdouble value) { glUniform1d( GetUniformLocation(uniform_name), value); }
        inline void SetUniformDouble2( const char *uniform_name, GLdouble value0, GLdouble value1) { glUniform2d( GetUniformLocation(uniform_name), value0, value1); }
        inline void SetUniformDouble3( const char *uniform_name, GLdouble value0, GLdouble value1, GLdouble value2) { glUniform3d( GetUniformLocation(uniform_name), value0, value1, value2); }
        inline void SetUniformDouble4( const char *uniform_name, GLdouble value0, GLdouble value1, GLdouble value2, GLdouble value3) { glUniform4d( GetUniformLocation(uniform_name), value0, value1, value2, value3); }

        inline void SetMatrix4x4( const char *uniform_name, GLboolean transpose, vmath::mat4 matrix) { glUniformMatrix4fv( GetUniformLocation(uniform_name), 1, transpose, matrix); }


        static GLuint CompileShader( const char *shader, GLenum shader_type);
};

