//Headers
#include <iostream>
#include <vector>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>

#ifdef _WIN32
    #include <Windows.h>
    
        //GLEW
    #include <GL/glew.h>
        //OpenGL
    #include <GL/gl.h>
#else
        //For XWindows API
    #include <X11/Xlib.h>       //For Xlib API
    #include <X11/Xutil.h>      //For Xlib Utility API
    #include <X11/XKBlib.h>     //For Xlib Keyboard API
    #include <X11/keysym.h>     //For Xlib Key Symbols
    
        //GLEW
    #include <GL/glew.h>
        //OpenGL
    #include <GL/gl.h>
        //GLX Bridging API
    #include <GL/glx.h>
#endif



//vao
struct Model_Data
{
    GLuint vao;
    GLuint vbo_position;
    GLuint vbo_texture;
    GLuint vbo_normals;
    GLuint vbo_elements;
    GLuint vbo_tangnet;
    GLint  numberOfElements;

    Model_Data()
    {
        this->vao = 0;
        this->vbo_position = 0;
        this->vbo_texture = 0;
        this->vbo_normals = 0;
        this->vbo_elements = 0;
        this->vbo_tangnet = 0;
        this->numberOfElements = 0;
    }

    void Delete()
    {
        if( this->vao)
        {
            glDeleteVertexArrays( 1, &this->vao);
            this->vao = 0;
        }
        if( this->vbo_position)
        {
            glDeleteBuffers( 1, &(this->vbo_position));
            this->vbo_position = 0;
        }
        if( this->vbo_texture)
        {
            glDeleteBuffers( 1, &(this->vbo_texture));
            this->vbo_texture = 0;
        }
        if( this->vbo_normals)
        {
            glDeleteBuffers( 1, &(this->vbo_normals));
            this->vbo_normals = 0;
        }
        if( this->vbo_tangnet)
        {
            glDeleteBuffers( 1, &(this->vbo_tangnet));
            this->vbo_tangnet = 0;
        }
        if( this->vbo_elements)
        {
            glDeleteBuffers( 1, &(this->vbo_elements));
            this->vbo_elements = 0;
        }

        this->numberOfElements = 0;
    }
};

void LoadOBJModel( const char *filename, Model_Data *model_data, int vertexIndex, int textureIndex, int normalIndex, int tangentIndex = -1);

