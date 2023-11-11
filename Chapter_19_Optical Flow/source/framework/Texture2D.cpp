
#include "../Common.h"
#include "Texture2D.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC        //make function as static
#include "../third_party/stb_image/stb_image.h"

/**
 * @brief CreateFromFile()
 * @param file_name 
 * @param flip_vertically 
 * @return 
 */
Texture2D* Texture2D::CreateFromFile( const char *file_name, bool flip_vertically)
{
    // variable declaration
    Texture2D *p_texture = nullptr;
    int width, height, channels;
    GLenum internal_format, format;
    unsigned char *image_data = nullptr;

    // code
    if( !file_name)
    {
        Log("Error: Invalid file name.");
        return nullptr;
    }

    if( flip_vertically)
    {
        stbi_set_flip_vertically_on_load( true);
    }

    image_data = stbi_load( file_name, &width, &height, &channels, 0);
    
    if( flip_vertically)
    {
        stbi_set_flip_vertically_on_load( false);
    }
    
    if( !image_data)
    {
        Log( "Error: Cannot read file \"%s\".", file_name);
        return nullptr;
    }

    switch( channels)
    {
        case 1:
            internal_format = GL_RED;
            format = GL_RED;
        break;

        case 2:
            internal_format = GL_RG;
            format = GL_RG;
        break;

        case 3:
            internal_format = GL_RGB;
            format = GL_RGB;
        break;

        case 4:
            internal_format = GL_RGBA;
            format = GL_RGBA;
        break;

        default:
            Log("Error: %d BitPerPixels not supported.", channels);
            stbi_image_free( image_data);
            return false;
    }

    p_texture = new Texture2D(
                        width, height,
                        internal_format, format, GL_UNSIGNED_BYTE,
                        GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT, image_data);

    stbi_image_free( image_data);
    image_data = nullptr;

    return p_texture;
}

/**
 * @brief Texture2D()
 * @param width 
 * @param height 
 * @param internal_format 
 * @param format 
 * @param type 
 * @param min_filter 
 * @param mag_filter 
 * @param wrap_s 
 * @param wrap_t 
 * @param data 
 */
Texture2D::Texture2D(
    unsigned int width, unsigned int height,
    GLenum internal_format, GLenum format, GLenum type,
    GLenum min_filter, GLenum mag_filter,
    GLenum wrap_s, GLenum wrap_t,
    const GLvoid *data
)
{
    // code
    m_width = width;
    m_height = height;
    m_internal_format = internal_format;
    m_format = format;

    glGenTextures( 1, &m_id);
    glBindTexture( GL_TEXTURE_2D, m_id);
        glTexImage2D( GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, data);
        
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);

        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
    glBindTexture( GL_TEXTURE_2D, 0);
}

/**
 * @brief ~Texture2D()
 */
Texture2D::~Texture2D()
{
    // code
    Release();
}

/**
 * @brief SetWrapParameter()
 * @param wrap_s 
 * @param wrap_t 
 */
void Texture2D::SetWrapParameter( GLenum wrap_s, GLenum wrap_t)
{
    // code
    glBindTexture( GL_TEXTURE_2D, m_id);
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
    glBindTexture( GL_TEXTURE_2D, 0);
}

/**
 * @brief SetFilterParameter()
 * @param min_filter 
 * @param mag_filter 
 */
void Texture2D::SetFilterParameter( GLenum min_filter, GLenum mag_filter)
{
    // code
    glBindTexture( GL_TEXTURE_2D, m_id);
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glBindTexture( GL_TEXTURE_2D, 0);
}

/**
 * @brief GenerateMipmap()
 */
void Texture2D::GenerateMipmap()
{
    // code
    glBindTexture( GL_TEXTURE_2D, m_id);
        glGenerateMipmap( GL_TEXTURE_2D);
    glBindTexture( GL_TEXTURE_2D, 0);
}

/**
 * @brief BindImage()
 * @param texture_uint 
 * @param access 
 * @param internal_formal 
 */
void Texture2D::BindImage( GLuint texture_uint, GLenum access, GLenum internal_format)
{
    // code
    glBindImageTexture( texture_uint, m_id, 0, GL_FALSE, 0, access, internal_format);
}

/**
 * @brief Release()
 */
void Texture2D::Release()
{
    // code
    m_width = 0;
    m_height = 0;
    m_internal_format = GL_NONE;
    m_format = GL_NONE;
    
    DELETE_TEXTURE( m_id);
}
