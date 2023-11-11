
#pragma once

    // GL_UNIFORM_BUFFER, GL_SHADER_STORAGE_BUFFER
class ResourceBuffer
{
    public:
        ResourceBuffer( GLenum buffer_type);
        ~ResourceBuffer();

        void UpdateData( void *data, size_t data_size, GLenum usage);
        void Bind( GLint binding_index);
        void Unbind(GLint binding_index);

        void* Map( GLenum map_access);
        void Unmap();

        bool IsValidBufferType();

        GLuint GetID()
        {
            return m_id;
        }

        GLenum GetBufferType()
        {
            return m_type;
        }

    private:
        GLuint m_id;
        GLenum m_type;
};
