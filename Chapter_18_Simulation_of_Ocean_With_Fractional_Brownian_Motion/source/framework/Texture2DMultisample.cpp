
#include "../Common.h"
#include "Texture2DMultisample.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC        //make function as static
#include "../third_party/stb_image/stb_image.h"

/**
 * @brief Texture2DMultisample()
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
Texture2DMultisample::Texture2DMultisample(
    unsigned int width, unsigned int height,
    GLenum internal_format, GLuint num_samples, GLboolean fixed_sample_location
)
{
    // code
    m_width = width;
    m_height = height;
    m_internal_format = internal_format;

    glGenTextures( 1, &m_id);
    glBindTexture( GL_TEXTURE_2D_MULTISAMPLE, m_id);
        glTexImage2DMultisample( GL_TEXTURE_2D_MULTISAMPLE, (GLsizei)num_samples, internal_format, width, height, fixed_sample_location);
    glBindTexture( GL_TEXTURE_2D_MULTISAMPLE, 0);
}

/**
 * @brief ~Texture2DMultisample()
 */
Texture2DMultisample::~Texture2DMultisample()
{
    // code
    Release();
}

/**
 * @brief Release()
 */
void Texture2DMultisample::Release()
{
    // code
    m_width = 0;
    m_height = 0;
    m_internal_format = GL_NONE;
    
    DELETE_TEXTURE( m_id);
}
