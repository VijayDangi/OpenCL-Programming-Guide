#pragma once

#ifdef _WIN32
    #include <Windows.h>
#endif

#include <GL/glew.h>    //Graphic Library Extension Wrangler
#include <gl/gl.h>  //OpenGL API

#include "vmath.h"  //Vaector and Matrix Related Operations

#include "LoadShaders.h"
#include "TextureLoading.h"


//macro defition
#define Log(format, ...)  PrintLog( __LINE__, __FILE__, __FUNCTION__, format, __VA_ARGS__)

#define LERP(A, B, t) ((1.0f-t)*(A) + t*(B))

#define DELETE_BUFFER( vbo) \
    if(vbo)  \
    {   \
        glDeleteBuffers( 1, &vbo); \
        vbo = 0; \
    }

#define DELETE_VERTEX_ARRAY( vao) \
    if(vao)  \
    {   \
        glDeleteVertexArrays( 1, &vao); \
        vao = 0; \
    }

#define DELETE_TEXTURE( tex) \
    if(tex)  \
    {   \
        glDeleteTextures( 1, &tex); \
        tex = 0; \
    }

//Single Vertex Attributes/Properties
enum ATTRIBUTE_INDEX
{
    POSITION = 0,
    COLOR,
    NORMAL,
    TEXCOORD2D,
    TANGENT,
    BITANGENT,
};

//type definition
struct Framebuffer
{
    GLuint framebuffer;
    GLuint color_attachment;
    GLuint depth_attachment;

    GLsizei width;
    GLsizei height;

    Framebuffer()
    {
        // code
        framebuffer = 0;
        color_attachment = 0;
        depth_attachment = 0;
    }

    ~Framebuffer()
    {
        // code
        Release();
    }

    void Release()
    {
        // code
        DELETE_TEXTURE( color_attachment);
        DELETE_TEXTURE( depth_attachment);

        if( framebuffer)
        {
            glDeleteFramebuffers( 1, &framebuffer);
            framebuffer = 0;
        }

        width = 0;
        height = 0;
    }
};

// extern variable
extern int g_window_width;
extern int g_window_height;

//global functions
void PrintLog( int lineNo, char *fileName, char *functionName, char *format, ...);

