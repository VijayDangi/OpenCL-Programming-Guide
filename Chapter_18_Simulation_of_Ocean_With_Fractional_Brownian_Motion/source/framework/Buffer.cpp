#include "..\Common.h"
#include "Buffer.h"

    /*______________________________ VertexBuffer ______________________________ */
/**
 * @brief VertexBuffer()
 * @param data 
 * @param data_size 
 * @param usage 
 */
VertexBuffer::VertexBuffer( void *data, GLsizei data_size, GLenum usage)
{
    // code
    glGenBuffers( 1, &m_id);
    Bind();
    glBufferData( GL_ARRAY_BUFFER, data_size, data, usage);
    Unbind();
}

/**
 * @brief ~VertexBuffer()
 */
VertexBuffer::~VertexBuffer()
{
    // code
    glDeleteBuffers( 1, &m_id);
    m_id = 0;
}


    /*______________________________ IndexBuffer ______________________________ */
/**
 * @brief IndexBuffer()
 * @param data 
 * @param data_size 
 * @param usage 
 */
IndexBuffer::IndexBuffer( void *data, GLsizei data_size, GLenum usage)
{
    // code
    glGenBuffers( 1, &m_id);
    Bind();
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, data_size, data, usage);
    Unbind();
}

/**
 * @brief ~IndexBuffer()
 */
IndexBuffer::~IndexBuffer()
{
    // code
    glDeleteBuffers( 1, &m_id);
    m_id = 0;
}


    /*______________________________ VertexArray ______________________________ */
/**
 * @brief VertexArray()
 */
VertexArray::VertexArray()
{
    // code
    glGenVertexArrays( 1, &m_id);
}

/**
 * @brief ~VertexArray()
 */
VertexArray::~VertexArray()
{
    // code
    glDeleteVertexArrays( 1, &m_id);
    m_id = 0;
}

/**
 * @brief AddVertexBuffer()
 * @param vbo 
 * @param attribute_index 
 * @param component_count 
 * @param data_type 
 * @param normalized 
 * @param stride 
 * @param offset 
 */
void VertexArray::AddVertexBuffer( VertexBuffer *vbo, GLuint attribute_index, GLint component_count, GLenum data_type, GLboolean normalized, GLsizei stride, size_t offset)
{
    // code
    Bind();
        vbo->Bind();
        glVertexAttribPointer( attribute_index, component_count, data_type, normalized, stride, (void*)offset);
        glEnableVertexAttribArray( attribute_index);
        vbo->Unbind();
    Unbind();
}

/**
 * @brief AddIndexBuffer()
 * @param ibo 
 */
void VertexArray::AddIndexBuffer( IndexBuffer *ibo)
{
    // code
    Bind();
        ibo->Bind();
    Unbind();
}

