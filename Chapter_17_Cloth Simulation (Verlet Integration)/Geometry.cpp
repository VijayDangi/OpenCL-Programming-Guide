#include "OGL.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>

#include "Geometry.h"

#define FREE_MEMORY(ptr) \
        if( ptr) \
        { \
            free(ptr); \
            ptr = nullptr; \
        }

// ---------------------------------------------------------------------- //
Geometry::Geometry()
{
    vertices_count = 0;
    indices_count = 0;

    index_type = GL_UNSIGNED_INT;

    vao = 0;
    vbo_position =0;
    vbo_normal =0;
    vbo_texcoord =0;
    vbo_tangent =0;
    vbo_bitangent =0;

    vbo_element = 0;
}

void Geometry::Release()
{
    vertices_count = 0;
    indices_count = 0;

    DELETE_VERTEX_ARRAY(vao);

    DELETE_BUFFER( vbo_position);
    DELETE_BUFFER( vbo_normal);
    DELETE_BUFFER( vbo_texcoord);
    DELETE_BUFFER( vbo_tangent);
    DELETE_BUFFER( vbo_bitangent);

    DELETE_BUFFER( vbo_element);
}

// ---------------------------------------------------------------------- //

static GLuint CreateArrayBuffer(void *data, int data_size, GLenum data_type, int number_of_component, int bind_index)
{
    //code
    GLuint vbo;

    glGenBuffers( 1, &vbo);
    glBindBuffer( GL_ARRAY_BUFFER, vbo);
        glBufferData( GL_ARRAY_BUFFER, data_size, data, GL_STATIC_DRAW);
        glVertexAttribPointer( bind_index, number_of_component, data_type, GL_FALSE, 0, 0);
        glEnableVertexAttribArray( bind_index);
    glBindBuffer( GL_ARRAY_BUFFER, 0);

    return( vbo);
}

static GLuint CreateElementArrayBuffer(void *data, int data_size)
{
    //code
    GLuint vbo;

    glGenBuffers( 1, &vbo);
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vbo);
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, data_size, data, GL_STATIC_DRAW);
    
    return( vbo);
}

static bool CalculateTangentAndBiTangent(
    unsigned int *indices, float *position, float *texcoord, int indices_count, int vertices_count,
    float *outTangent, float *outBitangent
)
{
    if (!indices || !position || !texcoord || !outTangent || !outBitangent)
    {
        Log("CalculateTangentAndBiTangent() null prameter not allowed.\n");
        return( false);
    }

    //casting
    vmath::vec3 *positionVector = (vmath::vec3 *)position;
    vmath::vec2 *textureVector = (vmath::vec2 *)texcoord;

    vmath::vec3 *tangentVector = (vmath::vec3 *)outTangent;
    vmath::vec3 *biTangentVector = (vmath::vec3 *)outBitangent;

    ZeroMemory(tangentVector, sizeof(vmath::vec3) * vertices_count);
    ZeroMemory(biTangentVector, sizeof(vmath::vec3) * vertices_count);

    //variable declaration
    vmath::vec3 v0, v1, v2;
    vmath::vec2 t0, t1, t2;

    vmath::vec3 edge0, edge1;
    vmath::vec2 delta0, delta1;

    vmath::vec3 tangent, biTangent;

    unsigned int index0, index1, index2;

    for (int i = 0; i < indices_count/3; i++)
    {
        //get triangle
        index0 = indices[3 * i + 0];
        index1 = indices[3 * i + 1];
        index2 = indices[3 * i + 2];

        //get position
        v0 = positionVector[index0];
        v1 = positionVector[index0];
        v2 = positionVector[index0];

        //get texcoord
        t0 = textureVector[index0];
        t1 = textureVector[index0];
        t2 = textureVector[index0];

        //
        edge0 = v1 - v0;
        edge1 = v2 - v0;

        //
        delta0 = t1 - t0;
        delta1 = t2 - t0;

        //
        float f = 1.0f / (delta0[0] * delta1[1] - delta0[1] * delta1[0]);

        //tangent and bitangent
        tangent[0] = f * (delta1[1] * edge0[0] - delta0[1] * edge1[0]);
        tangent[1] = f * (delta1[1] * edge0[1] - delta0[1] * edge1[1]);
        tangent[2] = f * (delta1[1] * edge0[2] - delta0[1] * edge1[2]);

        biTangent[0] = f * (delta0[0] * edge1[0] - delta1[0] * edge0[0]);
        biTangent[1] = f * (delta0[0] * edge1[1] - delta1[0] * edge0[1]);
        biTangent[2] = f * (delta0[0] * edge1[2] - delta1[0] * edge0[2]);

        //add tangent and bitanget
        tangentVector[index0] = tangentVector[index0] + tangent;
        tangentVector[index1] = tangentVector[index1] + tangent;
        tangentVector[index2] = tangentVector[index2] + tangent;

        biTangentVector[index0] = biTangentVector[index0] + biTangent;
        biTangentVector[index1] = biTangentVector[index1] + biTangent;
        biTangentVector[index2] = biTangentVector[index2] + biTangent;
    }

    //normalize tangent and bitangent
    for (int i = 0; i < vertices_count; i++)
    {
        tangentVector[i] = vmath::normalize(tangentVector[i]);
        biTangentVector[i] = vmath::normalize(biTangentVector[i]);
    }

    return(true);
}

//
//CreateSphereGeometry()
//
bool CreateSphereGeometry( Geometry* geometry, int slices, int stacks, float radius)
{
    //variable declarations
    float x, y, z;
    float u, v;
    int i, j;
    int indexV, indexI;

    int vertex_count;

    HRESULT hr;

    //code
    if (geometry == nullptr)
    {
        Log("Geometry Error: Null pointer.\n");
        return(false);
    }
    geometry->Release(); // RELASE BUFFERS IS PRVIOUSLY ALLOCATED

    if(radius <= 0.0f)
        radius = 1.0f;

    if(slices < 4)
        slices = 4;

    if(stacks < 3)
        stacks = 3;

    float du = 2 * M_PI / (float)( slices - 1);
    float dv = M_PI / (float)(stacks - 1);

    //Allocate memory
    vertex_count = slices * stacks;

    float *positions = (float*) calloc( 3 * vertex_count, sizeof(float));
    float *normals   = (float*) calloc( 3 * vertex_count, sizeof(float));
    float *textures  = (float*) calloc( 2 * vertex_count, sizeof(float));
    float *tangents  = (float*) calloc( 3 * vertex_count, sizeof(float));
    float *biTangents  = (float*) calloc( 3 * vertex_count, sizeof(float));
    unsigned int *indices = (unsigned int*) calloc( 2 * (slices - 1) * (stacks - 1) * 3, sizeof(unsigned int));

    if (!positions || !normals || !tangents || !textures || !indices || !biTangents)
    {
        FREE_MEMORY(positions);
        FREE_MEMORY(normals);
        FREE_MEMORY(tangents);
        FREE_MEMORY(textures);
        FREE_MEMORY(indices);
        FREE_MEMORY(biTangents);
        
        Log("Geometry Error: Memory allocation Failed.\n");
        return(false);
    }

    //fill arrays
    indexV = 0;
    for( i = 0; i < stacks; i++)
    {
        v =  M_PI/2.0f - i * dv;
        y = sin(v);

        for( j = 0; j < slices; j++)
        {
            u = j * du;
            x = sin(u) * cos(v);
            z = cos(u) * cos(v);

            positions[3*indexV + 0] = radius * x;
            positions[3*indexV + 1] = radius * y;
            positions[3*indexV + 2] = radius * z;

            normals[3*indexV + 0] = x;
            normals[3*indexV + 1] = y;
            normals[3*indexV + 2] = z;

            textures[2*indexV + 0] = (float)j/(float)(slices - 1);
            textures[2*indexV + 1] = (float)i/(float)(stacks - 1);

            indexV++;
        }
    }

    indexI = 0;
    for( j = 0; j < stacks-1; j++)
    {
        unsigned int row_one = j * slices;
        unsigned int row_two = (j + 1) * slices;

        for( i = 0; i < slices-1; i++)
        {
            indices[ indexI++] = row_one + i;
            indices[ indexI++] = row_two + i;
            indices[ indexI++] = row_two + i + 1;

            indices[ indexI++] = row_one + i;
            indices[ indexI++] = row_two + i + 1;
            indices[ indexI++] = row_one + i + 1;
        }
    }

    //calculate tangent
    if (CalculateTangentAndBiTangent(indices, positions, textures, indexI, vertex_count, tangents, biTangents) == false)
    {
        Log("Geometry Error: CalculateTangentAndBiTangent() Failed.\n");

        FREE_MEMORY( positions);
        FREE_MEMORY( normals);
        FREE_MEMORY( tangents);
        FREE_MEMORY( biTangents);
        FREE_MEMORY( textures);
        FREE_MEMORY( indices);

        geometry->vertices_count = 0;
        geometry->indices_count = 0;

        return(false);
    }



    int size = vertex_count * sizeof(float);

    glCreateVertexArrays( 1, &(geometry->vao));
    glBindVertexArray( geometry->vao);
        geometry->vbo_position  = CreateArrayBuffer( positions, 3 * size, GL_FLOAT, 3, ATTRIBUTE_INDEX::POSITION);
        geometry->vbo_normal    = CreateArrayBuffer(   normals, 3 * size, GL_FLOAT, 3, ATTRIBUTE_INDEX::NORMAL);
        geometry->vbo_texcoord  = CreateArrayBuffer(  textures, 2 * size, GL_FLOAT, 2, ATTRIBUTE_INDEX::TEXCOORD2D);
        geometry->vbo_tangent   = CreateArrayBuffer(  tangents, 3 * size, GL_FLOAT, 3, ATTRIBUTE_INDEX::TANGENT);
        geometry->vbo_bitangent = CreateArrayBuffer(biTangents, 3 * size, GL_FLOAT, 3, ATTRIBUTE_INDEX::BITANGENT);

        geometry->vbo_element = CreateElementArrayBuffer( indices, indexI * sizeof( unsigned int));
    glBindVertexArray( 0);

    geometry->vertices_count = indexV;
    geometry->indices_count = indexI;

    FREE_MEMORY( positions);
    FREE_MEMORY( normals);
    FREE_MEMORY( tangents);
    FREE_MEMORY( biTangents);
    FREE_MEMORY( textures);
    FREE_MEMORY( indices);

    return(true);
}

//
//CreateCylinderGeometry()
//
bool CreateCylinderGeometry( Geometry *geometry, int slices, int stacks, float radius, float height)
{
    //variable declarations
    float x, y, z;
    int i, j;
    int indexV, indexI;

    int vertex_count;

    HRESULT hr;

    //code
    if (geometry == nullptr)
    {
        Log("Geometry Error: Null pointer.\n");
        return(false);
    }
    geometry->Release(); // RELASE BUFFERS IS PRVIOUSLY ALLOCATED (IF ANY)

    if(radius <= 0.0f)
        radius = 1.0f;

    if(height <= 0.0f)
        height = 1.0f;

    if(slices < 4)
        slices = 4;

    if(stacks < 2)
        stacks = 2;

    //Allocate memory
    vertex_count = slices * stacks;

    float *positions = (float*) calloc( 3 * vertex_count, sizeof(float));
    float *normals   = (float*) calloc( 3 * vertex_count, sizeof(float));
    float *textures  = (float*) calloc( 2 * vertex_count, sizeof(float));
    float *tangents  = (float*) calloc( 3 * vertex_count, sizeof(float));
    float *biTangents  = (float*) calloc( 3 * vertex_count, sizeof(float));
    unsigned int *indices = (unsigned int*) calloc( 2 * (slices - 1) * (stacks - 1) * 3, sizeof(unsigned int));

    if (!positions || !normals || !tangents || !textures || !indices || !biTangents)
    {
        FREE_MEMORY(positions);
        FREE_MEMORY(normals);
        FREE_MEMORY(tangents);
        FREE_MEMORY(textures);
        FREE_MEMORY(indices);
        FREE_MEMORY(biTangents);
        
        Log("Geometry Error: Memory allocation Failed.\n");
        return(false);
    }

    float angle;

    //fill arrays
    indexV = 0;
    for( i = 0; i < stacks; i++)
    {
        for( j = 0; j < slices; j++)
        {
            angle = 2.0f * M_PI * (float)j / (float)(slices - 1);
            x = cos(angle);
            y = (float)i/(float)(stacks - 1);
            z = sin(angle);

            positions[3*indexV + 0] = radius * x;
            positions[3*indexV + 1] = y * height;
            positions[3*indexV + 2] = radius * z;

            normals[3*indexV + 0] = x;
            normals[3*indexV + 1] = 0.0f;
            normals[3*indexV + 2] = z;

            textures[2*indexV + 0] = (float)j/(float)( slices - 1);
            textures[2*indexV + 1] = (float)i/(float)( stacks - 1);

            indexV++;
        }
    }


    indexI = 0;
    for( j = 0; j < stacks - 1; j++)
    {
        unsigned int row_one = j * slices;
        unsigned int row_two = (j + 1) * slices;

        for( i = 0; i < slices - 1; i++)
        {
            indices[ indexI++] = row_one + i;
            indices[ indexI++] = row_two + i;
            indices[ indexI++] = row_two + i + 1;

            indices[ indexI++] = row_one + i;
            indices[ indexI++] = row_two + i + 1;
            indices[ indexI++] = row_one + i + 1;
        }
    }

    //calculate tangent
    if (CalculateTangentAndBiTangent(indices, positions, textures, indexI, vertex_count, tangents, biTangents) == false)
    {
        Log("Geometry Error: CalculateTangentAndBiTangent() Failed.\n");

        FREE_MEMORY( positions);
        FREE_MEMORY( normals);
        FREE_MEMORY( tangents);
        FREE_MEMORY( biTangents);
        FREE_MEMORY( textures);
        FREE_MEMORY( indices);

        geometry->vertices_count = 0;
        geometry->indices_count = 0;

        return(false);
    }


    int size = vertex_count * sizeof(float);

    glCreateVertexArrays( 1, &(geometry->vao));
    glBindVertexArray( geometry->vao);
        geometry->vbo_position  = CreateArrayBuffer( positions, 3 * size, GL_FLOAT, 3, ATTRIBUTE_INDEX::POSITION);
        geometry->vbo_normal    = CreateArrayBuffer(   normals, 3 * size, GL_FLOAT, 3, ATTRIBUTE_INDEX::NORMAL);
        geometry->vbo_texcoord  = CreateArrayBuffer(  textures, 2 * size, GL_FLOAT, 2, ATTRIBUTE_INDEX::TEXCOORD2D);
        geometry->vbo_tangent   = CreateArrayBuffer(  tangents, 3 * size, GL_FLOAT, 3, ATTRIBUTE_INDEX::TANGENT);
        geometry->vbo_bitangent = CreateArrayBuffer(biTangents, 3 * size, GL_FLOAT, 3, ATTRIBUTE_INDEX::BITANGENT);

        geometry->vbo_element = CreateElementArrayBuffer( indices, indexI * sizeof( unsigned int));
    glBindVertexArray( 0);

    geometry->vertices_count = indexV;
    geometry->indices_count = indexI;

    FREE_MEMORY( positions);
    FREE_MEMORY( normals);
    FREE_MEMORY( tangents);
    FREE_MEMORY( biTangents);
    FREE_MEMORY( textures);
    FREE_MEMORY( indices);

    return(true);
}

//
//CreateConeGeometry()
//
bool CreateConeGeometry( Geometry *geometry, int slices, int stacks, float baseRadius, float height)
{
    //variable declarations
    float x, y, z;
    int i, j;
    int indexV, indexI;

    int vertex_count;

    HRESULT hr;

    //code
    if (geometry == nullptr)
    {
        Log("Geometry Error: Null pointer.\n");
        return(false);
    }
    geometry->Release(); // RELASE BUFFERS IS PRVIOUSLY ALLOCATED (IF ANY)

    if(baseRadius <= 0.0f)
        baseRadius = 1.0f;

    if(height <= 0.0f)
        height = 1.0f;

    if(slices < 4)
        slices = 4;

    if(stacks < 2)
        stacks = 2;

    //Allocate memory
    vertex_count = slices * stacks;

    float *positions = (float*) calloc( 3 * vertex_count, sizeof(float));
    float *normals   = (float*) calloc( 3 * vertex_count, sizeof(float));
    float *textures  = (float*) calloc( 2 * vertex_count, sizeof(float));
    float *tangents  = (float*) calloc( 3 * vertex_count, sizeof(float));
    float *biTangents  = (float*) calloc( 3 * vertex_count, sizeof(float));
    unsigned int *indices = (unsigned int*) calloc( 2 * (slices - 1) * (stacks - 1) * 3, sizeof(unsigned int));

    if (!positions || !normals || !tangents || !textures || !indices || !biTangents)
    {
        FREE_MEMORY(positions);
        FREE_MEMORY(normals);
        FREE_MEMORY(tangents);
        FREE_MEMORY(textures);
        FREE_MEMORY(indices);
        FREE_MEMORY(biTangents);
        
        Log("Geometry Error: Memory allocation Failed.\n");
        return(false);
    }

    float angle;

    //fill arrays
    indexV = 0;
    for( i = 0; i < stacks ; i++)
    {
        float curRadius = baseRadius * ( 1.0f - (float)i/(float)(stacks - 1));
        for( j = 0; j < slices; j++)
        {
            angle = 2.0f * M_PI * (float)j / (float)(slices - 1);
            x = cos(angle);
            y = (float)i/(float)(stacks - 1);
            z = sin(angle);

            positions[3*indexV + 0] = curRadius * x;
            positions[3*indexV + 1] = y * height;
            positions[3*indexV + 2] = curRadius * z;

            normals[3*indexV + 0] = x;
            normals[3*indexV + 1] = 0.0f;
            normals[3*indexV + 2] = z;

            textures[2*indexV + 0] = (float)j/(float)( slices - 1);
            textures[2*indexV + 1] = (float)i/(float)( stacks - 1);

            indexV++;
        }
    }


    indexI = 0;
    for( j = 0; j < stacks - 1; j++)
    {
        unsigned int row_one = j * slices;
        unsigned int row_two = (j + 1) * slices;

        for( i = 0; i < slices - 1; i++)
        {
            indices[ indexI++] = row_one + i;
            indices[ indexI++] = row_two + i;
            indices[ indexI++] = row_two + i + 1;

            indices[ indexI++] = row_one + i;
            indices[ indexI++] = row_two + i + 1;
            indices[ indexI++] = row_one + i + 1;
        }
    }

    //calculate tangent
    if (CalculateTangentAndBiTangent(indices, positions, textures, indexI, vertex_count, tangents, biTangents) == false)
    {
        Log("Geometry Error: CalculateTangentAndBiTangent() Failed.\n");

        FREE_MEMORY( positions);
        FREE_MEMORY( normals);
        FREE_MEMORY( tangents);
        FREE_MEMORY( biTangents);
        FREE_MEMORY( textures);
        FREE_MEMORY( indices);

        geometry->vertices_count = 0;
        geometry->indices_count = 0;

        return(false);
    }



    int size = vertex_count * sizeof(float);

    glCreateVertexArrays( 1, &(geometry->vao));
    glBindVertexArray( geometry->vao);
        geometry->vbo_position  = CreateArrayBuffer( positions, 3 * size, GL_FLOAT, 3, ATTRIBUTE_INDEX::POSITION);
        geometry->vbo_normal    = CreateArrayBuffer(   normals, 3 * size, GL_FLOAT, 3, ATTRIBUTE_INDEX::NORMAL);
        geometry->vbo_texcoord  = CreateArrayBuffer(  textures, 2 * size, GL_FLOAT, 2, ATTRIBUTE_INDEX::TEXCOORD2D);
        geometry->vbo_tangent   = CreateArrayBuffer(  tangents, 3 * size, GL_FLOAT, 3, ATTRIBUTE_INDEX::TANGENT);
        geometry->vbo_bitangent = CreateArrayBuffer(biTangents, 3 * size, GL_FLOAT, 3, ATTRIBUTE_INDEX::BITANGENT);

        geometry->vbo_element = CreateElementArrayBuffer( indices, indexI * sizeof( unsigned int));
    glBindVertexArray( 0);

    geometry->vertices_count = indexV;
    geometry->indices_count = indexI;

    FREE_MEMORY( positions);
    FREE_MEMORY( normals);
    FREE_MEMORY( tangents);
    FREE_MEMORY( biTangents);
    FREE_MEMORY( textures);
    FREE_MEMORY( indices);
    
    return(true);
}

//
//CreateCircleGeometry()
//
bool CreateCircleGeometry( Geometry* geometry, int slices, float radius)
{
    //variable declarations
    float x, y, z;
    int j;
    int indexV, indexI;

    int vertex_count;

    HRESULT hr;

    //code
    if (geometry == nullptr)
    {
        Log("Geometry Error: Null pointer.\n");
        return(false);
    }
    geometry->Release(); // RELASE BUFFERS IS PRVIOUSLY ALLOCATED (IF ANY)

    if(radius <= 0.0f)
        radius = 1.0f;

    if(slices < 3)
        slices = 3;

    //Allocate memory
    vertex_count = slices + 1;

    float *positions = (float*) calloc( 3 * vertex_count, sizeof(float));
    float *normals   = (float*) calloc( 3 * vertex_count, sizeof(float));
    float *textures  = (float*) calloc( 2 * vertex_count, sizeof(float));
    float *tangents  = (float*) calloc( 3 * vertex_count, sizeof(float));
    float *biTangents  = (float*) calloc( 3 * vertex_count, sizeof(float));
    unsigned int *indices = (unsigned int*) calloc( slices * 3, sizeof(unsigned int));

    if (!positions || !normals || !tangents || !textures || !indices || !biTangents)
    {
        FREE_MEMORY(positions);
        FREE_MEMORY(normals);
        FREE_MEMORY(tangents);
        FREE_MEMORY(textures);
        FREE_MEMORY(indices);
        FREE_MEMORY(biTangents);
        
        Log("Geometry Error: Memory allocation Failed.\n");
        return(false);
    }

    float angle;

    //fill arrays
    indexV = 0;

    for( j = 0; j < slices; j++)
    {
        angle = 2.0f * M_PI * (float)j / (float)(slices - 1);
        x = cos(angle);
        y = 0.0f;
        z = sin(angle);

        positions[3*indexV + 0] = x;
        positions[3*indexV + 1] = y;
        positions[3*indexV + 2] = z;

        normals[3*indexV + 0] = 0.0f;
        normals[3*indexV + 1] = 1.0f;
        normals[3*indexV + 2] = 0.0f;

        textures[2*indexV + 0] = 0.5f + 0.5f * x;
        textures[2*indexV + 1] = 0.5f + 0.5f * z;

        indexV++;
    }

    //center of circle
    positions[3*indexV + 0] = 0.0f;
    positions[3*indexV + 1] = 0.0f;
    positions[3*indexV + 2] = 0.0f;

    normals[3*indexV + 0] = 0.0f;
    normals[3*indexV + 1] = 1.0f;
    normals[3*indexV + 2] = 0.0f;

    textures[2*indexV + 0] = 0.5f;
    textures[2*indexV + 1] = 0.5f;

    indexV++;


    indexI = 0;
    for( j = 0; j < slices; j++)
    {
        indices[ indexI++] = (j + 1) % slices;
        indices[ indexI++] = indexV - 1;
        indices[ indexI++] = j;
    }

    //calculate tangent
    if (CalculateTangentAndBiTangent(indices, positions, textures, indexI, vertex_count, tangents, biTangents) == false)
    {
        Log("Geometry Error: CalculateTangentAndBiTangent() Failed.\n");

        FREE_MEMORY( positions);
        FREE_MEMORY( normals);
        FREE_MEMORY( tangents);
        FREE_MEMORY( biTangents);
        FREE_MEMORY( textures);
        FREE_MEMORY( indices);

        geometry->vertices_count = 0;
        geometry->indices_count = 0;

        return(false);
    }


    int size = vertex_count * sizeof(float);

    glCreateVertexArrays( 1, &(geometry->vao));
    glBindVertexArray( geometry->vao);
        geometry->vbo_position  = CreateArrayBuffer( positions, 3 * size, GL_FLOAT, 3, ATTRIBUTE_INDEX::POSITION);
        geometry->vbo_normal    = CreateArrayBuffer(   normals, 3 * size, GL_FLOAT, 3, ATTRIBUTE_INDEX::NORMAL);
        geometry->vbo_texcoord  = CreateArrayBuffer(  textures, 2 * size, GL_FLOAT, 2, ATTRIBUTE_INDEX::TEXCOORD2D);
        geometry->vbo_tangent   = CreateArrayBuffer(  tangents, 3 * size, GL_FLOAT, 3, ATTRIBUTE_INDEX::TANGENT);
        geometry->vbo_bitangent = CreateArrayBuffer(biTangents, 3 * size, GL_FLOAT, 3, ATTRIBUTE_INDEX::BITANGENT);

        geometry->vbo_element = CreateElementArrayBuffer( indices, indexI * sizeof( unsigned int));
    glBindVertexArray( 0);

    geometry->vertices_count = indexV;
    geometry->indices_count = indexI;

    FREE_MEMORY(positions);
    FREE_MEMORY(normals);
    FREE_MEMORY(tangents);
    FREE_MEMORY(textures);
    FREE_MEMORY(indices);
    FREE_MEMORY(biTangents);

    return(true);
}

#undef HRESULT_ERROR_CHECK
#undef FREE_MEMORY
