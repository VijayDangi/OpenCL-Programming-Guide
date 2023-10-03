#include <Windows.h>

#include <GL/glew.h>    //Graphic Library Extension Wrang
#include <GL/GL.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC        //make function as static
#include "stb_image/stb_image.h"

//LoadTexture()
bool LoadTexture( GLuint *textureID, char *fileName)
{
    //variable declaration
    int width, height, channels;
    unsigned char *imageData;
    int components;
    int eFormat;
    
    //code
    if( fileName == NULL)
    {
        return( false);
    }
    
    imageData = stbi_load( fileName, &width, &height, &channels, 0);

    if(imageData)
    {
        if( channels == 1)
        {
            components = GL_RED;
            eFormat = GL_RED;
        }
        else if( channels == 3)
        {
            components = GL_RGB;
            eFormat = GL_RGB;
        }
        else if( channels == 4)
        {
            components = GL_RGBA;
            eFormat = GL_RGBA;
        }
        
        glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
        
        glGenTextures( 1, textureID);
        
        glBindTexture( GL_TEXTURE_2D, *textureID);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexImage2D( GL_TEXTURE_2D, 0, components, width, height, 0, eFormat, GL_UNSIGNED_BYTE, imageData );
            
            glGenerateMipmap( GL_TEXTURE_2D);
        glBindTexture( GL_TEXTURE_2D, 0);
        
        stbi_image_free( imageData);
    }
    else
    {
        return (false);
    }
    
    
    return(true);
}

