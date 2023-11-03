
#pragma once

class VertexBuffer
{
    public:
        VertexBuffer( void *data, GLsizei data_size, GLenum usage /* static, dynamic draw/copy etc.*/);
        ~VertexBuffer();

        void Bind()
        {
            glBindBuffer( GL_ARRAY_BUFFER, m_id);
        }

        void Unbind()
        {
            glBindBuffer( GL_ARRAY_BUFFER, 0);
        }

        GLuint GetID()
        {
            return m_id;
        }

    private:
        GLuint m_id;
};


class IndexBuffer
{
    public:
        IndexBuffer( void *data, GLsizei data_size, GLenum usage);
        ~IndexBuffer();

        void Bind()
        {
            glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_id);
        }

        void Unbind()
        {
            glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0);
        }

        GLuint GetID()
        {
            return m_id;
        }

    private:
        GLuint m_id;
};


class VertexArray
{
    public:
        VertexArray();
        ~VertexArray();

        void AddVertexBuffer( VertexBuffer *vbo, GLuint attribute_index, GLint component_count, GLenum data_type, GLboolean normalized, GLsizei stride, size_t offset);
        void AddIndexBuffer( IndexBuffer *ibo);

        void Bind()
        {
            glBindVertexArray( m_id);
        }

        void Unbind()
        {
            glBindVertexArray( 0);
        }

        GLuint GetID()
        {
            return m_id;
        }

    private:
        GLuint m_id;
};

