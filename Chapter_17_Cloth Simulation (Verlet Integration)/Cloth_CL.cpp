#include "OGL.h"

#include "OpenCLUtil.h"
#include <cl/cl_gl.h>

#include "Cloth_CL.h"
#include <vector>

#define CONTRAINT_SATISFY_ITERATION 32

#define FREE_MEMORY(ptr) \
        if(ptr) \
        {   \
            delete[](ptr); \
            ptr = nullptr; \
        }

#define CL_CHECK_ERROR(result, func_name)  \
            if( result != CL_SUCCESS) \
            { \
                Log( #func_name "() Failed(%d).", result); \
                return; \
            }

namespace ClothSimulation_OpenCL
{

    const vmath::vec4 gravity = vmath::vec4( 0.0f, -0.0098f, 0.0f, 0.0f);
    const float friction = 0.999f;
    const float bounce = 0.9f;


    cl_program ocl_cloth_program;
    cl_kernel ocl_update_vertices;
    cl_kernel ocl_constraint_vertices;
    cl_kernel ocl_update_sticks;

    //////////////////////////////////////////////
    ///////// TYPE DEFINITION
    //////////////////////////////////////////////
    enum STICK_GROUP_ID
    {
        HORIZONTAL_DISTANCE_1_EVEN = 0,
        HORIZONTAL_DISTANCE_1_ODD,
        VERTICAL_DISTANCE_1_EVEN,
        VERTICAL_DISTANCE_1_ODD,
        DIAGONAL_DISTANCE_1_EVEN,
        DIAGONAL_DISTANCE_1_ODD,

        HORIZONTAL_DISTANCE_2_EVEN,
        HORIZONTAL_DISTANCE_2_ODD,
        VERTICAL_DISTANCE_2_EVEN,
        VERTICAL_DISTANCE_2_ODD,
        DIAGONAL_DISTANCE_2_EVEN,
        DIAGONAL_DISTANCE_2_ODD,

        GROUP_COUNT
    };

    struct Stick
    {
        unsigned int p0;
        unsigned int p1;
    };

    struct _Cloth
    {
        GLuint vao = 0;
        GLuint vbo_position = 0;
        GLuint vbo_normal = 0;
        GLuint vbo_texcoord = 0;
        GLuint vbo_elements = 0;

        GLuint vertices_count = 0;
        GLuint indices_count = 0;

        float width = 0;
        float height = 0;

        GLuint vertices_x = 0;
        GLuint vertices_y = 0;

        float damping = 0.0f;
        float mass = 0.0f;

        std::vector<Stick> sticks[STICK_GROUP_ID::GROUP_COUNT];
        float stick_group_distance[STICK_GROUP_ID::GROUP_COUNT];

        cl_mem ocl_position_graphic_resource = nullptr;
        cl_mem ocl_old_position = nullptr;
        cl_mem ocl_p_fix_point = nullptr;
        cl_mem ocl_p_sticks[STICK_GROUP_ID::GROUP_COUNT] = { nullptr };

        ~_Cloth()
        {
            release();
        }

        void release()
        {
            DELETE_VERTEX_ARRAY( vao);
            DELETE_BUFFER(vbo_position);
            DELETE_BUFFER(vbo_normal);
            DELETE_BUFFER(vbo_texcoord);
            DELETE_BUFFER(vbo_elements);

            CL_OBJECT_RELEASE( ocl_position_graphic_resource, clReleaseMemObject);
            CL_OBJECT_RELEASE( ocl_old_position, clReleaseMemObject);
            CL_OBJECT_RELEASE( ocl_p_fix_point, clReleaseMemObject);

            for( int i = 0; i < STICK_GROUP_ID::GROUP_COUNT; ++i)
            {
                CL_OBJECT_RELEASE( ocl_p_sticks[i], clReleaseMemObject);
            }

            vertices_count = 0;
            indices_count = 0;
            width = 0;
            height = 0;
            vertices_x = 0;
            vertices_y = 0;
            damping = 0.0f;
            mass = 0.0f;

            for( int i = 0; i < STICK_GROUP_ID::GROUP_COUNT; ++i)
            {
                sticks[i].clear();
            }
        }
    };

    //////////////////////////////////////////////
    ////// VARIABLE DECLARATION
    //////////////////////////////////////////////

    static GLuint vertex_shader = 0;
    static GLuint fragment_shader = 0;

    //////////////////////////////////////////////
    ////// FUNCTION DECLARATION
    //////////////////////////////////////////////
    bool Initialize()
    {
        // variable declaration
        cl_int ocl_err;

        // code
        if( ! OpenCLUtil::IsInitialized())
        {
            return false;
        }

        ocl_cloth_program = OpenCLUtil::CreateProgram( "opencl_kernel/cloth/cloth.cl");
        if( ocl_cloth_program == nullptr)
        {
            Log( "OpenCLUtil::CreateProgram() Failed.");
            return false;
        }
        
        ocl_update_vertices = clCreateKernel( ocl_cloth_program, "update_vertices", &ocl_err);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clCreateKernel() Failed(%d).", ocl_err);
            return false;
        }

        ocl_constraint_vertices = clCreateKernel( ocl_cloth_program, "constraint_vertices", &ocl_err);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clCreateKernel() Failed(%d).", ocl_err);
            return false;
        }

        ocl_update_sticks = clCreateKernel( ocl_cloth_program, "update_sticks", &ocl_err);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clCreateKernel() Failed(%d).", ocl_err);
            return false;
        }

        return true;
    }

    Cloth CreateCloth(
        unsigned int cloth_width, unsigned int cloth_height,
        unsigned int x_vertices_count, unsigned int y_vertices_count,
        float damping, float mass
        )
    {
        // variable declaration
        vmath::vec4 *p_position = nullptr;
        vmath::vec4 *p_normal = nullptr;
        vmath::vec2 *p_texcoord = nullptr;
        bool *p_is_fixed_vertex = nullptr;
        unsigned int *p_indices = nullptr;

        _Cloth *cloth = nullptr;

        cl_int ocl_err;


        // code
        if( (cloth_width <= 0) || (cloth_height <= 0) || (x_vertices_count <= 0) || (y_vertices_count <= 0))
        {
            Log("Invalide Parameter");
            return nullptr;
        }
        Log("");

        // memory allocation
        size_t vertices_count = (size_t) x_vertices_count * y_vertices_count;
        size_t indices_count = 2 * 3 * (size_t)(x_vertices_count - 1) * (size_t)(y_vertices_count - 1);

#pragma region MEMORY_ALLOCATION
        p_position = new vmath::vec4[vertices_count];
        p_normal = new vmath::vec4[vertices_count];
        p_texcoord = new vmath::vec2[vertices_count];
        p_is_fixed_vertex = new bool[vertices_count];
        p_indices = new unsigned int[indices_count];
        cloth = new _Cloth;

        if( !p_position || !p_normal || !p_texcoord || !p_is_fixed_vertex || !p_indices || !cloth)
        {
            Log("Memory Allocation Failed");
            
            FREE_MEMORY( p_position);
            FREE_MEMORY( p_normal);
            FREE_MEMORY( p_texcoord);
            FREE_MEMORY( p_is_fixed_vertex);
            FREE_MEMORY( p_indices);

            if( cloth)
            {
                cloth->release();
            }

            FREE_MEMORY( cloth);

            return nullptr;
        }
#pragma endregion

Log("");


#pragma region BUFFER_INITIALIZE

        // cloth position, normal, texcoord
        for( int y = 0; y < y_vertices_count; ++y)
        {
            for( int x = 0; x < x_vertices_count; ++x)
            {
                int index = y * x_vertices_count + x;

                float u = (float) x / (float)(x_vertices_count - 1);
                float v = (float) y / (float)(y_vertices_count - 1);

                p_position[index][0] = cloth_width * u - cloth_width * 0.5f;
                p_position[index][1] = 0.0f;
                p_position[index][2] = cloth_height * v - cloth_height * 0.5f;
                p_position[index][3] = 1.0f;

                p_normal[index] = vmath::vec4( 0.0f, 1.0f, 0.0f, 1.0f);

                p_texcoord[index] = vmath::vec2( u, v);

                p_is_fixed_vertex[index] = false;
            }
        }
Log("");
        // fixed top side of cloth
        for( int x = 0; x < x_vertices_count; ++x)
        {
            p_is_fixed_vertex[x] = true;
        }
        Log("");
        // indices
        int indexPtr = 0;
        for( int x = 0; x < x_vertices_count - 1; ++x)
        {
            for( int y = 0; y < y_vertices_count - 1; ++y)
            {
                p_indices[ indexPtr++] =      y  * x_vertices_count +  x;
                p_indices[ indexPtr++] = (y + 1) * x_vertices_count +  x;
                p_indices[ indexPtr++] = (y + 1) * x_vertices_count + (x + 1);

                p_indices[ indexPtr++] =      y  * x_vertices_count +  x;
                p_indices[ indexPtr++] = (y + 1) * x_vertices_count + (x + 1);
                p_indices[ indexPtr++] =      y  * x_vertices_count + (x + 1);
            }
        }
Log("");
        ///////////////////////// constraint
        bool b_first_iteration = true;

#define ADD_STICK_TO( s, condition, if_true, if_false) \
            if( condition)  \
            {   \
                cloth->sticks[if_true].push_back(s);    \
            }   \
            else    \
            {   \
                cloth->sticks[if_false].push_back(s);   \
            }

        for( int x = 0; x < x_vertices_count; ++x)
        {
            for( int y = 0; y < y_vertices_count; ++y)
            {
                Stick s;

                //       *----------*
                if( x < (x_vertices_count - 1))
                {
                    if( b_first_iteration)
                    {
                        cloth->stick_group_distance[STICK_GROUP_ID::HORIZONTAL_DISTANCE_1_EVEN] = vmath::distance( p_position[0], p_position[1]);
                        cloth->stick_group_distance[STICK_GROUP_ID::HORIZONTAL_DISTANCE_1_ODD] = vmath::distance( p_position[0], p_position[1]);
                    }
                    
                    s.p0 = y * x_vertices_count + x;
                    s.p1 = y * x_vertices_count + (x + 1);

                    ADD_STICK_TO( s, (x % 2), STICK_GROUP_ID::HORIZONTAL_DISTANCE_1_ODD, STICK_GROUP_ID::HORIZONTAL_DISTANCE_1_EVEN);
                }

                //       *
                //       |
                //       |
                //       |
                //       *
                if( y < (y_vertices_count - 1))
                {
                    if( b_first_iteration)
                    {
                        cloth->stick_group_distance[STICK_GROUP_ID::VERTICAL_DISTANCE_1_EVEN] = vmath::distance( p_position[0], p_position[x_vertices_count]);
                        cloth->stick_group_distance[STICK_GROUP_ID::VERTICAL_DISTANCE_1_ODD] = vmath::distance( p_position[0], p_position[x_vertices_count]);
                    }
                    
                    s.p0 =      y  * x_vertices_count + x;
                    s.p1 = (y + 1) * x_vertices_count + x;

                    ADD_STICK_TO( s, (y % 2), STICK_GROUP_ID::VERTICAL_DISTANCE_1_ODD, STICK_GROUP_ID::VERTICAL_DISTANCE_1_EVEN);
                }

                //    *
                //     \
                //      \
                //       \
                //        *
                if( (x < (x_vertices_count - 1)) && (y < (y_vertices_count - 1)))
                {
                    if( b_first_iteration)
                    {
                        cloth->stick_group_distance[STICK_GROUP_ID::DIAGONAL_DISTANCE_1_EVEN] = vmath::distance( p_position[0], p_position[x_vertices_count + 1]);
                        cloth->stick_group_distance[STICK_GROUP_ID::DIAGONAL_DISTANCE_1_ODD] = vmath::distance( p_position[0], p_position[x_vertices_count + 1]);
                    }
                    
                    s.p0 =      y  * x_vertices_count + x;
                    s.p1 = (y + 1) * x_vertices_count + (x + 1);

                    ADD_STICK_TO( s, (y % 2), STICK_GROUP_ID::DIAGONAL_DISTANCE_1_ODD, STICK_GROUP_ID::DIAGONAL_DISTANCE_1_EVEN);
                }

                //       *----------*----------*
                if( (x < (x_vertices_count - 2)))
                {
                    if( b_first_iteration)
                    {
                        cloth->stick_group_distance[STICK_GROUP_ID::HORIZONTAL_DISTANCE_2_EVEN] = vmath::distance( p_position[0], p_position[2]);
                        cloth->stick_group_distance[STICK_GROUP_ID::HORIZONTAL_DISTANCE_2_ODD] = vmath::distance( p_position[0], p_position[2]);
                    }
                    
                    s.p0 = y * x_vertices_count + x;
                    s.p1 = y * x_vertices_count + (x + 2);

                    ADD_STICK_TO( s, ((x % 4) == 0) || ((x % 4) == 1), STICK_GROUP_ID::HORIZONTAL_DISTANCE_2_ODD, STICK_GROUP_ID::HORIZONTAL_DISTANCE_2_EVEN);
                }

                //       *
                //       |
                //       |
                //       |
                //       *
                //       |
                //       |
                //       |
                //       *
                if( y < (y_vertices_count - 2))
                {
                    if( b_first_iteration)
                    {
                        cloth->stick_group_distance[STICK_GROUP_ID::VERTICAL_DISTANCE_2_EVEN] = vmath::distance( p_position[0], p_position[2 * x_vertices_count]);
                        cloth->stick_group_distance[STICK_GROUP_ID::VERTICAL_DISTANCE_2_ODD] = vmath::distance( p_position[0], p_position[2 * x_vertices_count]);
                    }
                    
                    s.p0 =      y  * x_vertices_count + x;
                    s.p1 = (y + 2) * x_vertices_count + x;

                    ADD_STICK_TO( s, ((y % 4) == 0) || ((y % 4) == 1), STICK_GROUP_ID::VERTICAL_DISTANCE_2_ODD, STICK_GROUP_ID::VERTICAL_DISTANCE_2_EVEN);
                }

                //    *
                //     \
                //      \
                //       \
                //        *
                //         \
                //          \
                //           \
                //            *
                if( (x < (x_vertices_count - 2)) && (y < (y_vertices_count - 2)))
                {
                    if( b_first_iteration)
                    {
                        cloth->stick_group_distance[STICK_GROUP_ID::DIAGONAL_DISTANCE_2_EVEN] = vmath::distance( p_position[0], p_position[2 * x_vertices_count + 2]);
                        cloth->stick_group_distance[STICK_GROUP_ID::DIAGONAL_DISTANCE_2_ODD] = vmath::distance( p_position[0], p_position[2 * x_vertices_count + 2]);
                    }
                    
                    s.p0 =      y  * x_vertices_count + x;
                    s.p1 = (y + 2) * x_vertices_count + (x + 2);

                    ADD_STICK_TO( s, ((y % 4) == 0) || ((y % 4) == 1), STICK_GROUP_ID::DIAGONAL_DISTANCE_2_ODD, STICK_GROUP_ID::DIAGONAL_DISTANCE_2_EVEN);
                }

                b_first_iteration = false;
            }
        }

#undef ADD_STICK_TO

#pragma endregion

Log("");
#pragma region OPENGL_BUFFER

        glGenVertexArrays( 1, &(cloth->vao));
        glBindVertexArray( cloth->vao);

            // position
            glGenBuffers( 1, &(cloth->vbo_position));
            glBindBuffer( GL_ARRAY_BUFFER, cloth->vbo_position);
                glBufferData( GL_ARRAY_BUFFER, vertices_count * sizeof( vmath::vec4), &(p_position[0][0]), GL_DYNAMIC_DRAW);
                glVertexAttribPointer( ATTRIBUTE_INDEX::POSITION, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
                glEnableVertexAttribArray( ATTRIBUTE_INDEX::POSITION);
            glBindBuffer( GL_ARRAY_BUFFER, 0);

            // normal
            glGenBuffers( 1, &(cloth->vbo_normal));
            glBindBuffer( GL_ARRAY_BUFFER, cloth->vbo_normal);
                glBufferData( GL_ARRAY_BUFFER, vertices_count * sizeof( vmath::vec4), &(p_normal[0][0]), GL_DYNAMIC_DRAW);
                glVertexAttribPointer( ATTRIBUTE_INDEX::NORMAL, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
                glEnableVertexAttribArray( ATTRIBUTE_INDEX::NORMAL);
            glBindBuffer( GL_ARRAY_BUFFER, 0);

            // texcoord
            glGenBuffers( 1, &(cloth->vbo_texcoord));
            glBindBuffer( GL_ARRAY_BUFFER, cloth->vbo_texcoord);
                glBufferData( GL_ARRAY_BUFFER, vertices_count * sizeof( vmath::vec2), &(p_texcoord[0][0]), GL_DYNAMIC_DRAW);
                glVertexAttribPointer( ATTRIBUTE_INDEX::TEXCOORD2D, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
                glEnableVertexAttribArray( ATTRIBUTE_INDEX::TEXCOORD2D);
            glBindBuffer( GL_ARRAY_BUFFER, 0);

            // elements
            glGenBuffers( 1, &(cloth->vbo_elements));
            glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, cloth->vbo_elements);
            glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices_count * sizeof( unsigned int), p_indices, GL_DYNAMIC_DRAW);

        glBindVertexArray( 0);

#pragma endregion

Log("");

#pragma region OPENCL_BUFFER

        cloth->ocl_position_graphic_resource = clCreateFromGLBuffer( OpenCLUtil::GetContext(), CL_MEM_READ_WRITE, cloth->vbo_position, &ocl_err);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clCreateFromGLBuffer() Failed(%d).", ocl_err);
            
            FREE_MEMORY( p_position);
            FREE_MEMORY( p_normal);
            FREE_MEMORY( p_texcoord);
            FREE_MEMORY( p_is_fixed_vertex);
            FREE_MEMORY( p_indices);

            if( cloth)
            {
                cloth->release();
            }

            FREE_MEMORY( cloth);
        }

Log("");

        cloth->ocl_old_position = clCreateBuffer( OpenCLUtil::GetContext(), CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, vertices_count * sizeof( vmath::vec4), p_position, &ocl_err);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clCreateBuffer() Failed(%d).", ocl_err);
            
            FREE_MEMORY( p_position);
            FREE_MEMORY( p_normal);
            FREE_MEMORY( p_texcoord);
            FREE_MEMORY( p_is_fixed_vertex);
            FREE_MEMORY( p_indices);

            if( cloth)
            {
                cloth->release();
            }

            FREE_MEMORY( cloth);
        }

Log("");

        cloth->ocl_p_fix_point = clCreateBuffer( OpenCLUtil::GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, vertices_count * sizeof( bool), p_is_fixed_vertex, &ocl_err);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clCreateBuffer() Failed(%d).", ocl_err);
            
            FREE_MEMORY( p_position);
            FREE_MEMORY( p_normal);
            FREE_MEMORY( p_texcoord);
            FREE_MEMORY( p_is_fixed_vertex);
            FREE_MEMORY( p_indices);

            if( cloth)
            {
                cloth->release();
            }

            FREE_MEMORY( cloth);
        }

Log("");

        for( int i = 0; i < STICK_GROUP_ID::GROUP_COUNT; ++i)
        {
            cloth->ocl_p_sticks[i] = clCreateBuffer( OpenCLUtil::GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cloth->sticks[i].size() * sizeof( cloth->sticks[i][0]), cloth->sticks[i].data(), &ocl_err);
            if( ocl_err != CL_SUCCESS)
            {
                Log("clCreateBuffer() Failed(%d).", ocl_err);
                
                FREE_MEMORY( p_position);
                FREE_MEMORY( p_normal);
                FREE_MEMORY( p_texcoord);
                FREE_MEMORY( p_is_fixed_vertex);
                FREE_MEMORY( p_indices);

                if( cloth)
                {
                    cloth->release();
                }

                FREE_MEMORY( cloth);
            }
        }

#pragma endregion

Log("");
#pragma region CLOTH_INFO

        cloth->width = cloth_width;
        cloth->height = cloth_height;

        cloth->vertices_x = x_vertices_count;
        cloth->vertices_y = y_vertices_count;

        cloth->vertices_count = vertices_count;
        cloth->indices_count = indices_count;

        cloth->damping = damping;
        cloth->mass = mass;
#pragma endregion
Log("");
        // cleanup
        FREE_MEMORY( p_position);
        FREE_MEMORY( p_normal);
        FREE_MEMORY( p_texcoord);
        FREE_MEMORY( p_is_fixed_vertex);
        FREE_MEMORY( p_indices);

        return cloth;
    }

    void Render( Cloth cloth)
    {
        // code
        if( !cloth)
        {
            Log( "Invalid Parameter.");
            return;
        }

        glBindVertexArray( cloth->vao);
            glDrawElements( GL_TRIANGLES, cloth->indices_count, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray( 0);
    }

    void RenderVertices( Cloth cloth)
    {
        // code
        if( !cloth)
        {
            Log( "Invalid Parameter.");
            return;
        }

        glBindVertexArray( cloth->vao);
            glDrawArrays( GL_POINTS, 0, cloth->vertices_count);
        glBindVertexArray( 0);
    }

    static void UpdatePoints( Cloth cloth)
    {
        // variable declaration
        cl_int ocl_err;

        // code
        ocl_err = clSetKernelArg( ocl_update_vertices, 0, sizeof(cl_mem), &(cloth->ocl_position_graphic_resource));
        CL_CHECK_ERROR( ocl_err, clSetKernelArg);

        ocl_err = clSetKernelArg( ocl_update_vertices, 1, sizeof(cl_mem), &(cloth->ocl_old_position));
        CL_CHECK_ERROR( ocl_err, clSetKernelArg);

        ocl_err = clSetKernelArg( ocl_update_vertices, 2, sizeof(cl_mem), &(cloth->ocl_p_fix_point));
        CL_CHECK_ERROR( ocl_err, clSetKernelArg);

        ocl_err = clSetKernelArg( ocl_update_vertices, 3, sizeof(cl_float4), &(gravity[0]));
        CL_CHECK_ERROR( ocl_err, clSetKernelArg);

        ocl_err = clSetKernelArg( ocl_update_vertices, 4, sizeof(cl_float), &friction);
        CL_CHECK_ERROR( ocl_err, clSetKernelArg);

        ocl_err = clSetKernelArg( ocl_update_vertices, 5, sizeof(cl_uint), &(cloth->vertices_count));
        CL_CHECK_ERROR( ocl_err, clSetKernelArg);

        size_t global_work_size[] = { cloth->vertices_count};
        ocl_err = clEnqueueNDRangeKernel( OpenCLUtil::GetCommandQueue(), ocl_update_vertices, 1, nullptr, global_work_size, nullptr, 0, nullptr, nullptr);
        CL_CHECK_ERROR( ocl_err, clEnqueueNDRangeKernel);
    }

    static void ConstraintPoints( Cloth cloth, vmath::vec3 bound_dimension)
    {
        // variable declaration
        cl_int ocl_err;

        // code
        ocl_err = clSetKernelArg( ocl_constraint_vertices, 0, sizeof(cl_mem), &(cloth->ocl_position_graphic_resource));
        CL_CHECK_ERROR( ocl_err, clSetKernelArg);

        ocl_err = clSetKernelArg( ocl_constraint_vertices, 1, sizeof(cl_mem), &(cloth->ocl_old_position));
        CL_CHECK_ERROR( ocl_err, clSetKernelArg);

        ocl_err = clSetKernelArg( ocl_constraint_vertices, 2, sizeof(cl_mem), &(cloth->ocl_p_fix_point));
        CL_CHECK_ERROR( ocl_err, clSetKernelArg);

        ocl_err = clSetKernelArg( ocl_constraint_vertices, 3, sizeof(cl_float), &friction);
        CL_CHECK_ERROR( ocl_err, clSetKernelArg);

        ocl_err = clSetKernelArg( ocl_constraint_vertices, 4, sizeof(cl_float), &bounce);
        CL_CHECK_ERROR( ocl_err, clSetKernelArg);

        ocl_err = clSetKernelArg( ocl_constraint_vertices, 5, sizeof(cl_float3), &(bound_dimension[0]));
        CL_CHECK_ERROR( ocl_err, clSetKernelArg);

        ocl_err = clSetKernelArg( ocl_constraint_vertices, 6, sizeof(cl_uint), &(cloth->vertices_count));
        CL_CHECK_ERROR( ocl_err, clSetKernelArg);

        size_t global_work_size[] = { cloth->vertices_count};
        ocl_err = clEnqueueNDRangeKernel( OpenCLUtil::GetCommandQueue(), ocl_constraint_vertices, 1, nullptr, global_work_size, nullptr, 0, nullptr, nullptr);
        CL_CHECK_ERROR( ocl_err, clEnqueueNDRangeKernel);
    }

    static void UpdateSticks( Cloth cloth)
    {
        // variable declaration
        cl_int ocl_err;

        // code
        ocl_err = clSetKernelArg( ocl_update_sticks, 0, sizeof( cl_mem), &(cloth->ocl_position_graphic_resource));
        CL_CHECK_ERROR( ocl_err, clSetKernelArg);

        ocl_err = clSetKernelArg( ocl_update_sticks, 1, sizeof( cl_mem), &(cloth->ocl_p_fix_point));
        CL_CHECK_ERROR( ocl_err, clSetKernelArg);

            // sticks updates
        for( int i = 0; i < STICK_GROUP_ID::GROUP_COUNT; ++i)
        {
            unsigned int stick_count = cloth->sticks[i].size();

            ocl_err = clSetKernelArg( ocl_update_sticks, 2, sizeof( cl_mem), &(cloth->ocl_p_sticks[i]));
            CL_CHECK_ERROR( ocl_err, clSetKernelArg);

            ocl_err = clSetKernelArg( ocl_update_sticks, 3, sizeof( cl_float), &(cloth->stick_group_distance[i]));
            CL_CHECK_ERROR( ocl_err, clSetKernelArg);

            ocl_err = clSetKernelArg( ocl_update_sticks, 4, sizeof( cl_uint), &(stick_count));
            CL_CHECK_ERROR( ocl_err, clSetKernelArg);

            size_t global_work_size[] = { stick_count};

            ocl_err = clEnqueueNDRangeKernel( OpenCLUtil::GetCommandQueue(), ocl_update_sticks, 1, nullptr, global_work_size, nullptr, 0, nullptr, nullptr);
            CL_CHECK_ERROR( ocl_err, clEnqueueNDRangeKernel);
        }
    }
    

    void Update( Cloth cloth, float delta_time, vmath::vec3 bound_dimension)
    {
#define CL_CHECK_ERROR(result, func_name)  \
            if( result != CL_SUCCESS) \
            { \
                Log( #func_name "() Failed(%d).", result); \
                return; \
            }

        // variale declaration
        cl_int ocl_err;

        // code
        if( !cloth)
        {
            Log( "Invalid Parameter.");
            return;
        }

        ocl_err = clEnqueueAcquireGLObjects( OpenCLUtil::GetCommandQueue(), 1, &(cloth->ocl_position_graphic_resource), 0, nullptr, nullptr);
        CL_CHECK_ERROR( ocl_err, clEnqueueAcquireGLObjects);

        UpdatePoints( cloth);

        for( int i = 0; i < CONTRAINT_SATISFY_ITERATION; ++i)
        {
           ConstraintPoints( cloth, bound_dimension);
           UpdateSticks( cloth);
        }

        ocl_err = clEnqueueReleaseGLObjects( OpenCLUtil::GetCommandQueue(), 1, &(cloth->ocl_position_graphic_resource), 0, nullptr, nullptr);
        CL_CHECK_ERROR( ocl_err, clEnqueueReleaseGLObjects);
    }

    void DeleteCloth( Cloth cloth)
    {
        if( cloth)
        {
            cloth->release();
            delete cloth;
            cloth = nullptr;
        }
    }
    
    void Uninitialize()
    {
        CL_OBJECT_RELEASE( ocl_cloth_program, clReleaseProgram);
        CL_OBJECT_RELEASE( ocl_update_vertices, clReleaseKernel);
        CL_OBJECT_RELEASE( ocl_constraint_vertices, clReleaseKernel);
        CL_OBJECT_RELEASE( ocl_update_sticks, clReleaseKernel);
        Log("");
    }

} // namespace ClothSimulation_OpenCL

#undef CL_CHECK_ERROR
