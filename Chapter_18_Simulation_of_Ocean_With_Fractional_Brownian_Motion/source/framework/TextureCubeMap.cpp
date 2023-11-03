
#include "../Common.h"
#include "TextureCubeMap.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC        //make function as static
#include "../third_party/stb_image/stb_image.h"

#include <string>

/**
 * @brief CreateFromFile()
 * @param file_name 
 * @param flip_vertically 
 * @return 
 */
TextureCubeMap* TextureCubeMap::CreateFromFiles(
    const char *pos_x_file_name, const char *neg_x_file_name,
    const char *pos_y_file_name, const char *neg_y_file_name,
    const char *pos_z_file_name, const char *neg_z_file_name,
    bool flip_vertically
)
{
    // variable declaration
    TextureCubeMap *p_texture = nullptr;
    int width[6]{}, height[6]{}, channels;
    GLenum internal_format[6]{}, format[6]{};
    unsigned char *image_data[6]{};

    // code
    if( !pos_x_file_name || !neg_x_file_name ||
        !pos_y_file_name || !neg_y_file_name ||
        !pos_z_file_name || !neg_z_file_name
    )
    {
        Log("Error: Invalid file name.");
        return nullptr;
    }

    std::string file_name_array[] =
    {
        std::string( pos_x_file_name), std::string( neg_x_file_name),
        std::string( pos_y_file_name), std::string( neg_y_file_name),
        std::string( pos_z_file_name), std::string( neg_z_file_name)
    };

    /*
        #define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
        #define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
        #define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
        #define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
        #define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
        #define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
    */
    bool bFailed = false;

    for( int i = 0; i < 6; ++i)
    {
        if( flip_vertically)
        {
            stbi_set_flip_vertically_on_load( true);
        }

        image_data[i] = stbi_load( file_name_array[i].c_str(), &width[i], &height[i], &channels, 0);
        
        if( flip_vertically)
        {
            stbi_set_flip_vertically_on_load( false);
        }
        
        if( !image_data[i])
        {
            Log( "Error: Cannot read file \"%s\".", file_name_array[i].c_str());
            bFailed = true;
        }

        switch( channels)
        {
            case 1:
                internal_format[i] = GL_RED;
                format[i] = GL_RED;
            break;

            case 2:
                internal_format[i] = GL_RG;
                format[i] = GL_RG;
            break;

            case 3:
                internal_format[i] = GL_RGB;
                format[i] = GL_RGB;
            break;

            case 4:
                internal_format[i] = GL_RGBA;
                format[i] = GL_RGBA;
            break;

            default:
                Log("Error: %d BitPerPixels not supported.", channels);
                bFailed = true;
                break;
        }

        if( bFailed)
        {
            break;
        }
    }

    if( bFailed)
    {
        for( int i = 0; i < 6; ++i)
        {
            if( image_data[i])
            {
                stbi_image_free( image_data[i]);
                image_data[i] = nullptr;
            }
        }
        return nullptr;
    }

    bFailed = false;
    for( int i = 1; i < 6; ++i)
    {
        if( (width[i] != width[0]) || (height[i] != height[0]) ||
            (internal_format[i] != internal_format[0]) || (format[i] != format[0]) )
        {
            bFailed = true;
            Log( "Error: CubeMap all textures should have of same dimension/type/format");
            break;
        }
    }

    if( bFailed)
    {
        for( int i = 0; i < 6; ++i)
        {
            if( image_data[i])
            {
                stbi_image_free( image_data[i]);
                image_data[i] = nullptr;
            }
        }
        return nullptr;
    }

    p_texture = new TextureCubeMap();

    p_texture->AddTextureData( GL_TEXTURE_CUBE_MAP_POSITIVE_X, internal_format[0], format[0], GL_UNSIGNED_BYTE, width[0], height[0], image_data[0]);
    p_texture->AddTextureData( GL_TEXTURE_CUBE_MAP_NEGATIVE_X, internal_format[1], format[1], GL_UNSIGNED_BYTE, width[1], height[1], image_data[1]);
    p_texture->AddTextureData( GL_TEXTURE_CUBE_MAP_POSITIVE_Y, internal_format[2], format[2], GL_UNSIGNED_BYTE, width[2], height[2], image_data[2]);
    p_texture->AddTextureData( GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, internal_format[3], format[3], GL_UNSIGNED_BYTE, width[3], height[3], image_data[3]);
    p_texture->AddTextureData( GL_TEXTURE_CUBE_MAP_POSITIVE_Z, internal_format[4], format[4], GL_UNSIGNED_BYTE, width[4], height[4], image_data[4]);
    p_texture->AddTextureData( GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, internal_format[5], format[5], GL_UNSIGNED_BYTE, width[5], height[5], image_data[5]);

    for( int i = 0; i < 6; ++i)
    {
        if( image_data[i])
        {
            stbi_image_free( image_data[i]);
            image_data[i] = nullptr;
        }
    }

    return p_texture;
}

/**
 * @brief TextureCubeMap()
 */
TextureCubeMap::TextureCubeMap()
{
    // code
    glGenTextures( 1, &m_id);
}

/**
 * @brief ~TextureCubeMap()
 */
TextureCubeMap::~TextureCubeMap()
{
    // code
    m_width = 0;
    m_height = 0;
    m_internal_format = GL_NONE;
    m_format = GL_NONE;
    
    DELETE_TEXTURE( m_id);
}

/**
 * @brief AddTextureData()
 * @param tex_target 
 * @param internal_format 
 * @param format 
 * @param type 
 * @param width 
 * @param height 
 * @param data 
 */
bool TextureCubeMap::AddTextureData(
    GLenum tex_target, GLenum internal_format, GLenum format, GLenum type,
    int width, int height, const GLvoid *data
)
{
    // code
    if( !((tex_target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X) && (tex_target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)) )
    {
        Log("Error: Invalid tex_target for cubemap");
        return false;
    }

    m_width = width;
    m_height = height;
    m_internal_format = internal_format;
    m_format = format;

    glBindTexture( GL_TEXTURE_CUBE_MAP, m_id);
        glTexImage2D( tex_target, 0, internal_format, width, height, 0, format, type, data);
    glBindTexture( GL_TEXTURE_CUBE_MAP, 0);

    return true;
}

/**
 * @brief SetWrapParameter()
 * @param wrap_s 
 * @param wrap_t 
 */
void TextureCubeMap::SetWrapParameter( GLenum wrap_s, GLenum wrap_t)
{
    // code
    glBindTexture( GL_TEXTURE_CUBE_MAP, m_id);
        glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrap_s);
        glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrap_t);
    glBindTexture( GL_TEXTURE_CUBE_MAP, 0);
}

/**
 * @brief SetFilterParameter()
 * @param min_filter 
 * @param mag_filter 
 */
void TextureCubeMap::SetFilterParameter( GLenum min_filter, GLenum mag_filter)
{
    // code
    glBindTexture( GL_TEXTURE_CUBE_MAP, m_id);
        glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, min_filter);
        glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, mag_filter);
    glBindTexture( GL_TEXTURE_CUBE_MAP, 0);
}

/**
 * @brief GenerateMipmap()
 */
void TextureCubeMap::GenerateMipmap()
{
    // code
    glBindTexture( GL_TEXTURE_CUBE_MAP, m_id);
        glGenerateMipmap( GL_TEXTURE_CUBE_MAP);
    glBindTexture( GL_TEXTURE_CUBE_MAP, 0);
}

