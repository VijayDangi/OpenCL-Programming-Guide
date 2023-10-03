#include "OGL.h"
#include "OBJModel.h"

/**
    SPECIALLY CHANGE FOR "Toren1BD.obj" File
**/

extern FILE *gpfile;

struct VERTEX
{
    bool   is_set = false;
    float  position[3];
    int    textureIndex;
    int    normalIndex;
    int    index;
    int    nextIndex = -1; //work like linked-list but by index not by pointer
};

struct Vector3f
{
    union
    {
        float x;
        float r;
    };
    union
    {
        float y;
        float g;
    };
    union
    {
        float z;
        float b;
    };

    void normalize( void)
    {
        float length = sqrtf( x*x + y*y + z*z);

        if(length == 0)
        {
            x = y = z = 0.0f;
        }
        else
        {
            x = x / length;
            y = y / length;
            z = z / length;
        }
    }
};

struct Vector2f
{
    union
    {
        float x;
        float u;
    };
    union
    {
        float y;
        float v;
    };
};


#define BUFFER_SIZE 512

void LoadOBJModel( const char *filename, Model_Data *model_data, int vertexIndex, int textureIndex, int normalIndex, int tangentIndex)
{
    //variable declarations
    std::vector<struct VERTEX*> verticesArray;
    struct VERTEX *temp;

    std::vector<GLfloat> textures;
    std::vector<GLfloat> normals;
    std::vector<GLuint> indices;

    char line[BUFFER_SIZE];

    char *firstToken, *token;

    FILE *fp = NULL;

    GLfloat x, y, z;
    GLfloat u, v;
    GLfloat nx, ny, nz;

    GLfloat *vertex_array = NULL;
    GLfloat *texture_array = NULL;
    GLfloat *normal_array = NULL;
    GLfloat *tangent_array = NULL;
    
    std::vector<GLuint> verticesIndices;
    std::vector<GLuint> texturesIndices;
    std::vector<GLuint> normalsIndices;

    GLint vertex_count, indices_count;

    int vi, ti, ni;
    char *faceVertex[3];

    //code
    fp = fopen(filename, "r");
    if(fp == NULL)
    {
        Log( "(OBJModel.cpp) Error \"%s\"\n", filename);
        return;
    }


    while( fgets( line, BUFFER_SIZE, fp) != NULL)
    {
        firstToken = strtok( line, " ");

        if( strcmp(firstToken, "v") == 0)
        {
            x = atof( strtok( NULL, " "));
            y = atof( strtok( NULL, " "));
            z = atof( strtok( NULL, " "));

            temp = (struct VERTEX *) malloc( sizeof(struct VERTEX));

            temp->is_set = false;
            temp->nextIndex = -1;
            temp->textureIndex   = -1;
            temp->normalIndex    = -1;
            temp->position[0]    = x;
            temp->position[1]    = y;
            temp->position[2]    = z;
            temp->index = verticesArray.size();

            verticesArray.push_back( temp);
        }
        else if(strcmp(firstToken, "vt") == 0)
        {
            u = atof( strtok( NULL, " "));
            v = atof( strtok( NULL, " "));

            textures.push_back( u);
            textures.push_back( v);
        }
        else if(strcmp(firstToken, "vn") == 0)
        {
            nx = atof( strtok( NULL, " "));
            ny = atof( strtok( NULL, " "));
            nz = atof( strtok( NULL, " "));

            normals.push_back( nx);
            normals.push_back( ny);
            normals.push_back( nz);
        }
        else if(strcmp(firstToken, "f") == 0)
        {
            faceVertex[0] = strtok( NULL, " "); // v1/t1/n1
            faceVertex[1] = strtok( NULL, " "); // v2/t2/n2
            faceVertex[2] = strtok( NULL, " "); // v3/t3/n3

            
            for( int k = 0; k < 3; k++)
            {
                firstToken = strtok( faceVertex[k], "/"); // v
                vi = atoi(firstToken) - 1;
                ti = atoi( strtok( NULL, "/")) - 1;
                ni = atoi( strtok( NULL, "/")) - 1;
                
                verticesIndices.push_back( vi);
                texturesIndices.push_back( ti);
                normalsIndices.push_back( ni);
            }
        }
    }


    for( int i = 0; i < verticesIndices.size(); i++)
    {
        //break;
        
        vi = verticesIndices[i];
        ti = texturesIndices[i];
        ni = normalsIndices[i];

        temp = verticesArray[vi];
        if(temp->is_set == false)
        {
            temp->textureIndex = ti;
            temp->normalIndex = ni;
            temp->is_set = true;

            indices.push_back( vi);
        }
        else
        {
            if(temp->textureIndex == ti && temp->normalIndex == ni) //data already in indices
            {
                indices.push_back( vi);
            }
            else
            {
                bool found = false;

                while( temp->nextIndex != -1)
                {
                    temp = verticesArray[ temp->nextIndex];
                    if(temp->textureIndex == ti && temp->normalIndex == ni)
                    {
                        found = true;
                        indices.push_back( temp->index);
                        break;
                    }
                }

                if(found == false)
                {
                    struct VERTEX *temp_2 = (struct VERTEX *) malloc( sizeof(struct VERTEX));
                    temp_2->is_set = true;
                    temp_2->nextIndex = -1;
                    temp_2->textureIndex   = ti;
                    temp_2->normalIndex    = ni;
                    temp_2->position[0]    = temp->position[0];
                    temp_2->position[1]    = temp->position[1];
                    temp_2->position[2]    = temp->position[2];
                    temp_2->index = verticesArray.size();

                    temp->nextIndex = temp_2->index;   //store index where duplicate vertices present
                    verticesArray.push_back(temp_2);
                    indices.push_back( temp_2->index);
                }
            }
        }
    }

    //allocate memory for arrays
    vertex_array = (GLfloat *) malloc( sizeof(GLfloat) * verticesArray.size() * 3 );
    texture_array   = (GLfloat *) malloc( sizeof(GLfloat) * verticesArray.size() * 2 );
    normal_array   = (GLfloat *) malloc( sizeof(GLfloat) * verticesArray.size() * 3 );


    //convert to array
    for( int i = 0; i < verticesArray.size(); i++)
    {
        
        ti = verticesArray[i]->textureIndex;
        ni = verticesArray[i]->normalIndex;

        *(vertex_array + 3*i + 0) = verticesArray[i]->position[0];
        *(vertex_array + 3*i + 1) = verticesArray[i]->position[1];
        *(vertex_array + 3*i + 2) = verticesArray[i]->position[2];

        *(texture_array + 2*i + 0) = textures[2*ti + 0];
        *(texture_array + 2*i + 1) = textures[2*ti + 1];

        *(normal_array + 3*i + 0) = normals[3*ni + 0];
        *(normal_array + 3*i + 1) = normals[3*ni + 1];
        *(normal_array + 3*i + 2) = normals[3*ni + 2];
    }

    if(tangentIndex != -1)
    {
        Vector3f v0, v1, v2;
        Vector2f tex0, tex1, tex2;
        Vector3f tangent;

        Vector3f edge0, edge1;
        Vector2f delta1, delta2;

        GLuint index0, index1, index2;

        tangent_array = (GLfloat *) malloc( sizeof(GLfloat) * verticesArray.size() * 3 );
        memset( (void *) tangent_array, 0, sizeof(GLfloat) * verticesArray.size() * 3 );

        for( int i = 0; i < indices.size(); i += 3)
        {
            index0 = indices[i];
            index1 = indices[i + 1];
            index2 = indices[i + 2];

            v0.x = vertex_array[ 3 * index0 + 0];
            v0.y = vertex_array[ 3 * index0 + 1];
            v0.z = vertex_array[ 3 * index0 + 2];

            v1.x = vertex_array[ 3 * index1 + 0];
            v1.y = vertex_array[ 3 * index1 + 1];
            v1.z = vertex_array[ 3 * index1 + 2];

            v2.x = vertex_array[ 3 * index2 + 0];
            v2.y = vertex_array[ 3 * index2 + 1];
            v2.z = vertex_array[ 3 * index2 + 2];


            tex0.u = texture_array[ 2 * index0 + 0];
            tex0.v = texture_array[ 2 * index0 + 1];

            tex1.u = texture_array[ 2 * index1 + 0];
            tex1.v = texture_array[ 2 * index1 + 1];

            tex2.u = texture_array[ 2 * index2 + 0];
            tex2.v = texture_array[ 2 * index2 + 1];


            edge0.x = v1.x - v0.x;
            edge0.y = v1.y - v0.y;
            edge0.z = v1.z - v0.z;

            edge1.x = v2.x - v0.x;
            edge1.y = v2.y - v0.y;
            edge1.z = v2.z - v0.z;


            delta1.u = tex1.u - tex0.u;
            delta1.v = tex1.v - tex0.v;

            delta2.u = tex2.u - tex0.u;
            delta2.v = tex2.v - tex0.v;

            float f = 1.0f / ( delta1.u * delta2.v - delta2.u * delta1.v);

            tangent.x = f * ( delta2.v * edge0.x - delta1.v * edge1.x);
            tangent.y = f * ( delta2.v * edge0.y - delta1.v * edge1.y);
            tangent.z = f * ( delta2.v * edge0.z - delta1.v * edge1.z);


            tangent_array[ 3 * index0 + 0] += tangent.x;
            tangent_array[ 3 * index0 + 1] += tangent.y;
            tangent_array[ 3 * index0 + 2] += tangent.z;

            tangent_array[ 3 * index1 + 0] += tangent.x;
            tangent_array[ 3 * index1 + 1] += tangent.y;
            tangent_array[ 3 * index1 + 2] += tangent.z;

            tangent_array[ 3 * index2 + 0] += tangent.x;
            tangent_array[ 3 * index2 + 1] += tangent.y;
            tangent_array[ 3 * index2 + 2] += tangent.z;
        }

        //Normlize tangent
        for( int i = 0; i < verticesArray.size(); i++)
        {
            tangent.x = tangent_array[ 3 * i + 0];
            tangent.y = tangent_array[ 3 * i + 1];
            tangent.z = tangent_array[ 3 * i + 2];

            tangent.normalize();

            tangent_array[ 3 * i + 0] = tangent.x;
            tangent_array[ 3 * i + 1] = tangent.y;
            tangent_array[ 3 * i + 2] = tangent.z;
        }
    }
    
    // struct VEC3
    // {
    //     float x, y, z;
    // };
    
    // //Calculate Normals
    // for( int i = 0 ; i < indices.size()/3; i++)
    // {
    //     VEC3 p0, p1, p2;
    //     float normal[3];
    //     int index[3];
        
    //     index[0] = indices[ 3 * i + 0];
    //     index[1] = indices[ 3 * i + 1];
    //     index[2] = indices[ 3 * i + 2];
        
    //     p0.x = *(vertex_array + 3*index[0] + 0);
    //     p0.y = *(vertex_array + 3*index[0] + 1);
    //     p0.z = *(vertex_array + 3*index[0] + 2);
        
    //     p1.x = *(vertex_array + 3*index[1] + 0);
    //     p1.y = *(vertex_array + 3*index[1] + 1);
    //     p1.z = *(vertex_array + 3*index[1] + 2);
        
    //     p2.x = *(vertex_array + 3*index[2] + 0);
    //     p2.y = *(vertex_array + 3*index[2] + 1);
    //     p2.z = *(vertex_array + 3*index[2] + 2);
        
    //     VEC3 v1, v2;
        
    //     v1.x = p1.x - p0.x;
    //     v1.y = p1.y - p0.y;
    //     v1.z = p1.z - p0.z;
        
    //     v2.x = p2.x - p0.x;
    //     v2.y = p2.y - p0.y;
    //     v2.z = p2.z - p0.z;
        
    //     VEC3 v1Xv2;
    //     v1Xv2.x =    v1.y * v2.z - v1.z * v2.y;
    //     v1Xv2.y = - (v1.x * v2.z - v1.z * v2.x);
    //     v1Xv2.z =    v1.x * v2.y - v1.y * v2.x;
        
    //     float length = (float) sqrt( v1Xv2.x*v1Xv2.x + v1Xv2.y*v1Xv2.y + v1Xv2.z*v1Xv2.z);
        
    //     normal[0] = v1Xv2.x/length;
    //     normal[1] = v1Xv2.y/length;
    //     normal[2] = v1Xv2.z/length;
        
        
    //     *(normal_array + 3*index[0] + 0) = normal[0];
    //     *(normal_array + 3*index[0] + 1) = normal[1];
    //     *(normal_array + 3*index[0] + 2) = normal[2];
        
    //     *(normal_array + 3*index[1] + 0) = normal[0];
    //     *(normal_array + 3*index[1] + 1) = normal[1];
    //     *(normal_array + 3*index[1] + 2) = normal[2];
        
    //     *(normal_array + 3*index[2] + 0) = normal[0];
    //     *(normal_array + 3*index[2] + 1) = normal[1];
    //     *(normal_array + 3*index[2] + 2) = normal[2];
    // }

    vertex_count = verticesArray.size();
    indices_count = indices.size();

    Log( "\n\n------------ %s ------------\n", filename);
    Log( "Vertex Array Size :\t %d\n", vertex_count*3);
    Log( "Number Of vertex  :\t %d\n", vertex_count);

    Log( "Texture Array Size:\t %d\n", vertex_count*2);
    Log( "Number Of texture :\t %d\n", vertex_count);

    Log( "Normals Array Size:\t %d\n", vertex_count*3);
    Log( "Number Of normal  :\t %d\n", vertex_count);

    if( tangentIndex != -1)
    {
        Log( "Tangents Array Size:\t %d\n", vertex_count*3);
        Log( "Number Of tangent  :\t %d\n", vertex_count);
    }

    Log( "Indices           :\t %d\n", indices_count);
    Log( "Faces             :\t %d\n", indices_count/3);
    Log( "\n----------------------------\n\n", filename);

/*
struct Model_Data
{
    GLuint vao;
    GLuint vbo_position;
    GLuint vbo_texture;
    GLuint vbo_normals;
    GLuint vbo_elements;
    GLint  numberOfElements;
};
*/

    glGenVertexArrays( 1, &model_data->vao);
    glBindVertexArray( model_data->vao);

        //position
        glGenBuffers( 1, &model_data->vbo_position);
        glBindBuffer( GL_ARRAY_BUFFER, model_data->vbo_position);

            glBufferData( GL_ARRAY_BUFFER, 3 * vertex_count * sizeof(GLfloat), vertex_array, GL_STATIC_DRAW);
            glVertexAttribPointer( vertexIndex, 3, GL_FLOAT, GL_FALSE, 0, NULL);
            glEnableVertexAttribArray( vertexIndex);

        glBindBuffer( GL_ARRAY_BUFFER, 0);

            //texture
        glGenBuffers( 1, &model_data->vbo_texture);
        glBindBuffer( GL_ARRAY_BUFFER, model_data->vbo_texture);

            glBufferData( GL_ARRAY_BUFFER, 2 * vertex_count * sizeof(GLfloat), texture_array, GL_STATIC_DRAW);
            glVertexAttribPointer( textureIndex, 2, GL_FLOAT, GL_FALSE, 0, NULL);
            glEnableVertexAttribArray( textureIndex);

        glBindBuffer( GL_ARRAY_BUFFER, 0);

            //normals
        glGenBuffers( 1, &model_data->vbo_normals);
        glBindBuffer( GL_ARRAY_BUFFER, model_data->vbo_normals);

            glBufferData( GL_ARRAY_BUFFER, 3 * vertex_count * sizeof(GLfloat), normal_array, GL_STATIC_DRAW);
            glVertexAttribPointer( normalIndex, 3, GL_FLOAT, GL_FALSE, 0, NULL);
            glEnableVertexAttribArray( normalIndex);

        glBindBuffer( GL_ARRAY_BUFFER, 0);

            //tangent space
        if( tangentIndex != -1)
        {
            glGenBuffers( 1, &model_data->vbo_tangnet);
            glBindBuffer( GL_ARRAY_BUFFER, model_data->vbo_tangnet);

                glBufferData( GL_ARRAY_BUFFER, 3 * vertex_count * sizeof(GLfloat), tangent_array, GL_STATIC_DRAW);
                glVertexAttribPointer( tangentIndex, 3, GL_FLOAT, GL_FALSE, 0, NULL);
                glEnableVertexAttribArray( tangentIndex);

            glBindBuffer( GL_ARRAY_BUFFER, 0);
        }

        //Elements
        glGenBuffers( 1, &model_data->vbo_elements);
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, model_data->vbo_elements);
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices_count * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

        //glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0);

    glBindVertexArray( 0);


    //save indices count
    model_data->numberOfElements = indices_count;



    //clean-up
    fclose( fp);
    fp = NULL;

    //clear vectors
    textures.clear();
    normals.clear();
    indices.clear();
    
    verticesIndices.clear();
    texturesIndices.clear();
    normalsIndices.clear();

    //free memory
    for(int i = 0; i < verticesArray.size(); i++)
    {
            //clear memory allocated to each member of vector
        free(verticesArray[i]);
        verticesArray[i] = NULL;
    }

    verticesArray.clear();

    //free memroy of arrays
    if( vertex_array)
    {
        free( vertex_array);
        vertex_array = NULL;
    }

    if( texture_array)
    {
        free( texture_array);
        texture_array = NULL;
    }

    if( normal_array)
    {
        free( normal_array);
        normal_array = NULL;
    }


    if( tangent_array)
    {
        free( tangent_array);
        tangent_array = NULL;
    }
}




/*

    ( x1,y1)   (x2,y2) 
        --------
        |     /|
        |    / |
        |   /  |
        |  /   |
        | /    |
        --------
    (x3, y3)  (x4,y4)


    
    x1,y1  x2,y2  x3,y3
    x2,y2  x3,y3  x4,y4


  
    12 * 4 = 48
    12 * 4 = 48
    (6 * 3) * 4 = 96

    192

--------------------------------------------

    x1,y1   x2,y2  x3,y3  x4,y4
    8*4       = 32
    8*4       = 32
    (4*3) * 4 = 48
                24

                136

    1, 2, 3
    2, 3, 4

    8*4 + 6*4 = 32 + 24 = 56
*/


