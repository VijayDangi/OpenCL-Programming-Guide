#include "OGL.h"
#include "Grid.h"

//GRID
#define GRID_SIZE 100
#define GRID_VERTICES_COUNT ( 2 * 2 * GRID_SIZE)

static GLuint gridProgram;

static GLuint vaoGrid;
static GLuint vboGrid;

static GLint uniform_grid_MVPMatrix;
static GLint uniform_grid_MVMatrix;
static GLint uniform_grid_MaxDistance;
static GLint uniform_grid_Color;


bool InitializeGrid( void)
{
    SHADERS_INFO shaderInfo[2];
    ZeroMemory( shaderInfo, sizeof(shaderInfo));

    shaderInfo[0].shaderFileName = "shaders/grid/vertex_shader.glsl";
    shaderInfo[0].shaderLoadAs = SHADER_INFO_LOAD_FROM_FILE;
    shaderInfo[0].shaderType = GL_VERTEX_SHADER;

    shaderInfo[1].shaderFileName = "shaders/grid/fragment_shader.glsl";
    shaderInfo[1].shaderLoadAs = SHADER_INFO_LOAD_FROM_FILE;
    shaderInfo[1].shaderType = GL_FRAGMENT_SHADER;


    BIND_ATTRIBUTES_INFO bindAttributesInfo[1];
    ZeroMemory( bindAttributesInfo, sizeof(bindAttributesInfo));

    bindAttributesInfo[0].attribute = "vPosition";
    bindAttributesInfo[0].index = ATTRIBUTE_INDEX::POSITION;

    gridProgram = CreateProgram( shaderInfo, _ARRAYSIZE(shaderInfo), bindAttributesInfo, _ARRAYSIZE(bindAttributesInfo), nullptr);
    if( gridProgram == 0)
    {
        return (false);
    }

    Log("Grid Uniform \"MVPMatrix\" : %d", uniform_grid_MVPMatrix = glGetUniformLocation( gridProgram, "MVPMatrix"));
    Log("Grid Uniform \"MVMatrix\" : %d", uniform_grid_MVMatrix = glGetUniformLocation( gridProgram, "MVMatrix"));
    Log("Grid Uniform \"MaxDistance\" : %d", uniform_grid_MaxDistance = glGetUniformLocation( gridProgram, "MaxDistance"));
    Log("Grid Uniform \"Color\" : %d", uniform_grid_Color = glGetUniformLocation( gridProgram, "Color"));


    //vertex array
    float gridPosition[GRID_VERTICES_COUNT * 3];

    int vertexPtr = 0;

    for( int y = 0; y < GRID_SIZE; y++)
    {
        //Horizontal
        gridPosition[ 3 * vertexPtr + 0] = -GRID_SIZE/2;
        gridPosition[ 3 * vertexPtr + 1] = 0.0f;
        gridPosition[ 3 * vertexPtr + 2] = y - GRID_SIZE/2;

        vertexPtr++;

        gridPosition[ 3 * vertexPtr + 0] = GRID_SIZE/2 - 1;
        gridPosition[ 3 * vertexPtr + 1] = 0.0f;
        gridPosition[ 3 * vertexPtr + 2] = y - GRID_SIZE/2;

        vertexPtr++;
    }

    for( int x = 0; x < GRID_SIZE; x++)
    {
        //Vertical
        gridPosition[ 3 * vertexPtr + 0] = x - GRID_SIZE/2;
        gridPosition[ 3 * vertexPtr + 1] = 0.0f;
        gridPosition[ 3 * vertexPtr + 2] = -GRID_SIZE/2;

        vertexPtr++;

        gridPosition[ 3 * vertexPtr + 0] = x - GRID_SIZE/2;
        gridPosition[ 3 * vertexPtr + 1] = 0.0f;
        gridPosition[ 3 * vertexPtr + 2] = GRID_SIZE/2 - 1;

        vertexPtr++;
    }

    glGenVertexArrays( 1, &vaoGrid);
    glGenBuffers( 1, &vboGrid);

    glBindVertexArray( vaoGrid);
        glBindBuffer( GL_ARRAY_BUFFER, vboGrid);
            glBufferData( GL_ARRAY_BUFFER, sizeof(gridPosition), gridPosition, GL_STATIC_DRAW);
            glVertexAttribPointer( ATTRIBUTE_INDEX::POSITION, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray( ATTRIBUTE_INDEX::POSITION);
        glBindBuffer( GL_ARRAY_BUFFER, 0);
    glBindVertexArray( 0);
    
    return(true);
}

void ResizeGrid( int width, int height)
{

}

void RenderGrid( vmath::mat4 projectionMatrix, vmath::mat4 viewMatrix)
{
    //variable declaration
    vmath::mat4 modelMatrix = vmath::mat4::identity();

    //code
        //Save state
    GLint blendSrcFunc;
    GLint blendDstFunc;
    GLboolean blendState;

    GLint polygonMode;

    glGetIntegerv( GL_BLEND_SRC_ALPHA, &blendSrcFunc);
    glGetIntegerv( GL_BLEND_DST_ALPHA, &blendDstFunc);
    blendState = glIsEnabled(GL_BLEND);

    glGetIntegerv( GL_POLYGON_MODE, &polygonMode);

    glEnable( GL_BLEND);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        //Do Work
    glUseProgram( gridProgram);
        glUniformMatrix4fv( uniform_grid_MVPMatrix, 1, GL_FALSE, projectionMatrix * viewMatrix * modelMatrix);
        glUniformMatrix4fv( uniform_grid_MVMatrix, 1, GL_FALSE, viewMatrix * modelMatrix);
        glUniform4f( uniform_grid_Color, 1.0f, 1.0f, 1.0f, 1.0f);
        glUniform1f( uniform_grid_MaxDistance, GRID_SIZE);

        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
        glEnable( GL_LINE_SMOOTH);
        glEnable( GL_POINT_SMOOTH);

        glBindVertexArray( vaoGrid);
            glPointSize( 2.0f);
            glDrawArrays( GL_LINES, 0, GRID_VERTICES_COUNT);
        glBindVertexArray( 0);
    glUseProgram( 0);


        //Reset State
    glPolygonMode( GL_FRONT_AND_BACK, polygonMode);

    if(blendState)
    {
        glEnable(GL_BLEND);
    }
    else
    {
        glDisable(GL_BLEND);
    }

    glBlendFunc( blendSrcFunc, blendDstFunc);
}

void UpdateGrid(double deltaTime)
{

}

void UninitializeGrid( void)
{
    DELETE_BUFFER( vboGrid);
    DELETE_VERTEX_ARRAY( vaoGrid);
    
    if( gridProgram)
    {
        DeleteProgram( gridProgram);
        gridProgram = 0;
    }
}

#undef GRID_SIZE
#undef GRID_VERTICES_COUNT
