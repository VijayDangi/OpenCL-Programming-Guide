#include "Common.h"
#include "Grid.h"
#include "framework/ShaderProgram.h"
#include "framework/Buffer.h"

//GRID
#define GRID_SIZE 100
#define GRID_VERTICES_COUNT ( 2 * 2 * GRID_SIZE)

ShaderProgram *gridProgram = nullptr;

static VertexArray *vaoGrid = nullptr;
static VertexBuffer *vboGrid = nullptr;

bool InitializeGrid( void)
{
    // code
    gridProgram = new ShaderProgram;
    if( gridProgram == nullptr)
    {
        Log("Memory Allocation Failed.");
        return false;
    }

    if( gridProgram->AddShaderFromFile( "resource/shaders/grid/vertex_shader.glsl", GL_VERTEX_SHADER) == false)
    {
        return false;
    }

    if( gridProgram->AddShaderFromFile( "resource/shaders/grid/fragment_shader.glsl", GL_FRAGMENT_SHADER) == false)
    {
        return false;
    }

    gridProgram->BindVertexAttributeLocation( "vPosition", ATTRIBUTE_INDEX::POSITION);

    if( gridProgram->Build() == false)
    {
        Log( "Grid Program Failed.");
        return false;
    }

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

    
    vboGrid = new VertexBuffer( (void *) gridPosition, sizeof( gridPosition), GL_STATIC_DRAW);

    vaoGrid = new VertexArray;
    vaoGrid->AddVertexBuffer( vboGrid, ATTRIBUTE_INDEX::POSITION, 3, GL_FLOAT, GL_FALSE, 0, 0);
    
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

    glGetIntegerv( GL_BLEND_SRC_ALPHA, &blendSrcFunc);
    glGetIntegerv( GL_BLEND_DST_ALPHA, &blendDstFunc);
    blendState = glIsEnabled(GL_BLEND);

    glEnable( GL_BLEND);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        //Do Work
    if( gridProgram)
    {
        gridProgram->Bind();
            gridProgram->SetMatrix4x4( "MVPMatrix", GL_FALSE, projectionMatrix * viewMatrix * modelMatrix);
            gridProgram->SetMatrix4x4( "MVMatrix", GL_FALSE, projectionMatrix * viewMatrix);
            gridProgram->SetUniformFloat4( "Color", 1.0f, 1.0f, 1.0f, 1.0f);
            gridProgram->SetUniformFloat( "MaxDistance", GRID_SIZE);

            vaoGrid->Bind();
                glDrawArrays( GL_LINES, 0, GRID_VERTICES_COUNT);
            vaoGrid->Unbind();
        gridProgram->Unbind();
    }

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
    if( vboGrid)
    {
        delete vboGrid;
        vboGrid = nullptr;
    }

    if( vaoGrid)
    {
        delete vaoGrid;
        vaoGrid = nullptr;
    }

    
    if( gridProgram)
    {
        gridProgram->Release();
        delete gridProgram;
        gridProgram = nullptr;
    }
}

#undef GRID_SIZE
#undef GRID_VERTICES_COUNT
