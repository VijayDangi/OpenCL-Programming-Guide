#include "..\Common.h"
#include "ResourceBuffer.h"

/**
 * @brief ResourceBuffer()
 * @param buffer_type 
 */
ResourceBuffer::ResourceBuffer( GLenum buffer_type)
{
    // code
    m_type = buffer_type;

    if( !IsValidBufferType())
    {
        Log("Invalid Buffer Resource Type : (%d).", buffer_type);
        m_type = GL_NONE;
        return;
    }

    glGenBuffers( 1, &m_id);
}

/**
 * @brief ~ResourceBuffer()
 */
ResourceBuffer::~ResourceBuffer()
{
    // code
    DELETE_BUFFER( m_id);
}

/**
 * @brief UpdateData()
 * @param data 
 * @param data_size 
 * @param usage 
 */
void ResourceBuffer::UpdateData( void *data, size_t data_size, GLenum usage)
{
    // code
    if( !IsValidBufferType())
    {
        Log("Invalid Buffer Resource Type : (%d).", m_type);
        return;
    }

    glBindBuffer( m_type, m_id);
    glBufferData( m_type, data_size, data, usage);
    glBindBuffer( m_type, 0);
}

/**
 * @brief Bind()
 * @param binding_index 
 */
void ResourceBuffer::Bind( GLint binding_index)
{
    // code
    if( !IsValidBufferType())
    {
        Log("Invalid Buffer Resource Type : (%d).", m_type);
        return;
    }

    glBindBufferBase( m_type, binding_index, m_id);
}

/**
 * @brief Unbind()
 * @param binding_index 
 */
void ResourceBuffer::Unbind(GLint binding_index)
{
    // code
    glBindBufferBase( m_type, binding_index, 0);
    glBindBuffer( m_type, 0);
}

/**
 * @brief Map()
 * @return 
 */
void* ResourceBuffer::Map( GLenum map_access)
{
    // code
    if( !IsValidBufferType())
    {
        Log("Invalid Buffer Resource Type : (%d).", m_type);
        return nullptr;
    }

    glBindBuffer( m_type, m_id);
    return glMapBuffer( m_type, map_access);
}

/**
 * @brief Unmap()
 */
void  ResourceBuffer::Unmap()
{
    // code
    glUnmapBuffer( m_type);
    glBindBuffer( m_type, 0);
}

/**
 * @brief IsValidBufferType()
 * @return 
 */
bool ResourceBuffer::IsValidBufferType()
{
    // code
    switch( m_type)
    {
        case GL_UNIFORM_BUFFER:
        case GL_SHADER_STORAGE_BUFFER:
            return true;
        
        default:
            return false;
    }

    return false;
}
