#pragma once

#include <stdio.h>
#include <stdlib.h>

struct Geometry
{
    int vertices_count;
    int indices_count;
    GLenum index_type;

    GLuint vao;

    GLuint vbo_position;
    GLuint vbo_normal;
    GLuint vbo_texcoord;
    GLuint vbo_tangent;
    GLuint vbo_bitangent;

    GLuint vbo_element;

    Geometry();
    void Release(void);
};

//function declarations
bool CreateSphereGeometry( Geometry *geometry, int slices = 4, int stacks = 3, float radius = 1.0f);
bool CreateCylinderGeometry( Geometry *geometry, int slices = 4, int stacks = 2, float radius = 1.0f, float height = 1.0f);
bool CreateConeGeometry( Geometry *geometry, int slices = 4, int stacks = 2, float baseRadius = 1.0f, float height = 1.0f);
bool CreateCircleGeometry( Geometry *geometry, int slices = 4, float radius = 1.0f);

