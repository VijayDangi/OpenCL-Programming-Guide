

#include "Common.h"
#include "SceneWaves.h"

#include <vector>

#include "framework/ShaderProgram.h"
#include "framework/Buffer.h"
#include "framework/Texture2D.h"
#include "framework/ResourceBuffer.h"
#include "framework/TextureCubeMap.h"

#include "system/FileIO.h"

#include "utility/OpenCLUtil.h"
#include <cl/cl_gl.h>

#include <string>
#include <sstream>

#define GRID_DIMENSION 2048

#define DATA_FILE_PATH "resource/data/waves.ini"

namespace SceneWaves
{
    struct WaveProperty
    {
        float u_wave_direction_seed_param[2];
        float u_wave_frequency_param[2];
        float u_wave_amplitude_param[2];
        float u_wave_speed_param[2];
        float u_wave_drag;
        float u_wave_height;
        float u_wave_max_peak;
        float u_wave_peak_offset;
        int u_wave_count;
    };

    static VertexArray *pGridVao = nullptr;
    static VertexBuffer *pGridVbo_Position = nullptr;
    static VertexBuffer *pGridVbo_Normal = nullptr;
    static VertexBuffer *pGridVbo_Texcoord = nullptr;
    static IndexBuffer *pGridIbo = nullptr;
    static Texture2D *pFoamTexture[2] = {nullptr};

    static ShaderProgram *pWavesProgram = nullptr;

    static const int vertex_count = ( GRID_DIMENSION + 1) * (GRID_DIMENSION + 1);
    static const int indices_count = 2 * 3 * GRID_DIMENSION * GRID_DIMENSION;

    static float _time = 0.0f;

        // Light
    static vmath::vec3 u_light_color( 1.0f, 1.0f, 1.0f);

        // Mateial
    static vmath::vec3 u_ambient( 0.1f, 0.1f, 0.1f);
    static vmath::vec3 u_diffuse_reflectance(0.000f, 0.329f, 0.800f);
    static vmath::vec3 u_specular_reflectance( 1.0f, 1.0f, 1.0f);
    static float u_specular_normal_strength = 1.0f;
    static float u_shininess = 1.0f;

        // Fresnel
    static float u_fresnel_normal_strength = 1.0f;
    static float u_fresnel_shininess = 1.0f;
    static float u_fresnel_bias = 1.0f;
    static float u_fresnel_strength = 1.0f;
    static vmath::vec3  u_fresnel_color(1.0f, 1.0f, 1.0f);

    static int u_use_environment_map = 0;

        // Wave Tip
    static vmath::vec3 u_tip_color( 1.0f, 1.0f, 1.0f);
    static float u_tip_attenuation = 1.0f;

    static int u_vertex_wave_count = 8;

    static float u_wave_direction_seed = 0;
    static float u_wave_direction_seed_iter = 1253.2131;
    static float u_wave_frequency = 1.0;
    static float u_wave_frequency_mult = 1.18;
    static float u_wave_amplitude = 1.0;
    static float u_wave_amplitude_mult = 0.82;
    static float u_wave_initial_speed = 2.0;
    static float u_wave_speed_ramp = 1.07;
    static float u_wave_drag = 1.0;
    static float u_wave_height = 1.0;
    static float u_wave_max_peak = 1.0;
    static float u_wave_peak_offset = 1.0;
    static float u_normal_strength = 1.0f;


        // opencl
    cl_program ocl_wave_fbm_program = nullptr;
    cl_kernel ocl_wave_fbm_kernel = nullptr;
    cl_mem ocl_position_buffer = nullptr;
    cl_mem ocl_normal_buffer = nullptr;
    cl_mem ocl_wave_parameter = nullptr;
    cl_mem ocl_foam_image[2] = {nullptr};

    struct WaveProperty opencl_wave_property;

    bool read = 0;


    void InitializeVariables()
    {
            // LOAD FILE DATA
        std::string data;
        if( FileIO::ReadFile( DATA_FILE_PATH, data))
        {
            std::string line;
            std::string token_key;
            std::string token_value;

            std::stringstream sd(data);

#define READ_TOKEN_FLOAT( x, token)    \
        else if( TO_STRING( x) == token_key) \
        {   \
            getline( ss, token_value, token); \
            x = atof( token_value.c_str()); \
        }

#define READ_TOKEN_FLOAT2( x, token)    \
        else if( TO_STRING( x) == token_key) \
        {   \
            getline( ss, token_value, token); \
            x[0] = atof( token_value.c_str()); \
            \
            getline( ss, token_value, token); \
            x[1] = atof( token_value.c_str()); \
        }

#define READ_TOKEN_FLOAT3( x, token)    \
        else if( TO_STRING( x) == token_key) \
        {   \
            getline( ss, token_value, token); \
            x[0] = atof( token_value.c_str()); \
            \
            getline( ss, token_value, token); \
            x[1] = atof( token_value.c_str()); \
            \
            getline( ss, token_value, token); \
            x[2] = atof( token_value.c_str()); \
        }

#define READ_TOKEN_INT( x, token)    \
        else if( TO_STRING( x) == token_key) \
        {   \
            getline( ss, token_value, token); \
            x = atoi( token_value.c_str()); \
        }

            while( getline( sd, line, '\n'))
            {
                // get token
                std::stringstream ss(line);
                getline( ss, token_key, '=');

                if( 0 == line.size())
                {
                    continue;
                }

                    // WAVES
                READ_TOKEN_INT(u_vertex_wave_count, ',')
                READ_TOKEN_FLOAT( u_wave_direction_seed, ',')
                READ_TOKEN_FLOAT( u_wave_direction_seed_iter, ',')
                READ_TOKEN_FLOAT( u_wave_frequency, ',')
                READ_TOKEN_FLOAT( u_wave_frequency_mult, ',')
                READ_TOKEN_FLOAT( u_wave_amplitude, ',')
                READ_TOKEN_FLOAT( u_wave_amplitude_mult, ',')
                READ_TOKEN_FLOAT( u_wave_initial_speed, ',')
                READ_TOKEN_FLOAT( u_wave_speed_ramp, ',')
                READ_TOKEN_FLOAT( u_wave_drag, ',')
                READ_TOKEN_FLOAT( u_wave_height, ',')
                READ_TOKEN_FLOAT( u_wave_max_peak, ',')
                READ_TOKEN_FLOAT( u_wave_peak_offset, ',')
                READ_TOKEN_FLOAT( u_normal_strength, ',')
                
                    // Light
                READ_TOKEN_FLOAT3( u_light_color, ',')

                    // Mateial
                READ_TOKEN_FLOAT3( u_ambient, ',')
                READ_TOKEN_FLOAT3( u_diffuse_reflectance, ',')
                READ_TOKEN_FLOAT3( u_specular_reflectance, ',')
                READ_TOKEN_FLOAT( u_specular_normal_strength, ',')
                READ_TOKEN_FLOAT( u_shininess, ',')

                    // Fresnel
                READ_TOKEN_FLOAT( u_fresnel_normal_strength, ',')
                READ_TOKEN_FLOAT( u_fresnel_shininess, ',')
                READ_TOKEN_FLOAT( u_fresnel_bias, ',')
                READ_TOKEN_FLOAT( u_fresnel_strength, ',')
                READ_TOKEN_FLOAT3( u_fresnel_color, ',')
                READ_TOKEN_INT( u_use_environment_map, ',')

                    // Wave Tip
                READ_TOKEN_FLOAT3( u_tip_color, ',')
                READ_TOKEN_FLOAT( u_tip_attenuation, ',')
            }
        
        
#undef READ_TOKEN_FLOAT
#undef READ_TOKEN_FLOAT2
#undef READ_TOKEN_FLOAT3
#undef READ_TOKEN_INT

        }
    }

    /**
     * @brief Initialize()
     */
    bool Initialize()
    {
        // variable declaration
        std::vector<vmath::vec4> position_v(vertex_count);
        std::vector<vmath::vec4> normal_v( vertex_count);
        std::vector<vmath::vec2> texcoord_v(vertex_count);
        std::vector<GLuint> indices_v(indices_count);

        // code
        InitializeVariables();

        opencl_wave_property.u_wave_direction_seed_param[0] = u_wave_direction_seed;
        opencl_wave_property.u_wave_direction_seed_param[1] = u_wave_direction_seed_iter;
        opencl_wave_property.u_wave_frequency_param[0] = u_wave_frequency;
        opencl_wave_property.u_wave_frequency_param[1] = u_wave_frequency_mult;
        opencl_wave_property.u_wave_amplitude_param[0] = u_wave_amplitude;
        opencl_wave_property.u_wave_amplitude_param[1] = u_wave_amplitude_mult;
        opencl_wave_property.u_wave_speed_param[0] = u_wave_initial_speed;
        opencl_wave_property.u_wave_speed_param[1] = u_wave_speed_ramp;
        opencl_wave_property.u_wave_drag = u_wave_drag;
        opencl_wave_property.u_wave_height = u_wave_height;
        opencl_wave_property.u_wave_max_peak = u_wave_max_peak;
        opencl_wave_property.u_wave_peak_offset = u_wave_peak_offset;
        opencl_wave_property.u_wave_count = u_vertex_wave_count;


        unsigned int idx = 0;
        for( int z  = -GRID_DIMENSION / 2; z <= GRID_DIMENSION / 2; ++z)
        {
            for( int x  = -GRID_DIMENSION / 2; x <= GRID_DIMENSION / 2; ++x)
            {
                position_v[idx] = vmath::vec4( (float)x, 0.0f, (float)z, 1.0f);
                normal_v[idx] = vmath::vec4( 0.0f, 1.0f, 0.0f, 1.0f);

                float u = ((float)x / (float)GRID_DIMENSION) + 0.5f;
                float v = ((float)z / (float)GRID_DIMENSION) + 0.5f;

                texcoord_v[idx] = vmath::vec2( u, v);
                idx++;
            }
        }
        assert( idx == position_v.size());

            // indices
        idx = 0;
        for( GLuint y = 0; y < GRID_DIMENSION; ++y)
        {
            for( GLuint x = 0; x < GRID_DIMENSION; ++x)
            {
                indices_v[idx++] =      y  * (GRID_DIMENSION + 1) +  x;
                indices_v[idx++] = (y + 1) * (GRID_DIMENSION + 1) +  x;
                indices_v[idx++] =      y  * (GRID_DIMENSION + 1) + (x + 1);

                indices_v[idx++] =      y  * (GRID_DIMENSION + 1) + (x + 1);
                indices_v[idx++] = (y + 1) * (GRID_DIMENSION + 1) +  x;
                indices_v[idx++] = (y + 1) * (GRID_DIMENSION + 1) + (x + 1);
            }
        }
        assert( idx == indices_v.size());


            // BUFFER
        pGridVbo_Position = new VertexBuffer( position_v.data(), sizeof( vmath::vec4) * position_v.size(), GL_STATIC_DRAW);
        if( pGridVbo_Position == nullptr)
        {
            Log("Memory Allocation Failed.");
            return false;
        }

        pGridVbo_Normal = new VertexBuffer( normal_v.data(), sizeof( vmath::vec4) * normal_v.size(), GL_STATIC_DRAW);
        if( pGridVbo_Normal == nullptr)
        {
            Log("Memory Allocation Failed.");
            return false;
        }

        pGridVbo_Texcoord = new VertexBuffer( texcoord_v.data(), sizeof( vmath::vec2) * texcoord_v.size(), GL_STATIC_DRAW);
        if( pGridVbo_Texcoord == nullptr)
        {
            Log("Memory Allocation Failed.");
            return false;
        }

        pGridIbo = new IndexBuffer( indices_v.data(), sizeof( GLuint) * indices_v.size(), GL_STATIC_DRAW);
        if( pGridIbo == nullptr)
        {
            Log("Memory Allocation Failed.");
            return false;
        }

        pGridVao = new VertexArray();
        if( pGridVao == nullptr)
        {
            Log("Memory Allocation Failed.");
            return false;
        }

        pGridVao->AddVertexBuffer( pGridVbo_Position, ATTRIBUTE_INDEX::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof( vmath::vec4), 0);
        pGridVao->AddVertexBuffer( pGridVbo_Normal, ATTRIBUTE_INDEX::NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof( vmath::vec4), 0);
        pGridVao->AddVertexBuffer( pGridVbo_Texcoord, ATTRIBUTE_INDEX::TEXCOORD2D, 2, GL_FLOAT, GL_FALSE, 0, 0);
        pGridVao->AddIndexBuffer( pGridIbo);


            // texture
        for( int i = 0; i < 2; ++i)
        {
            pFoamTexture[i] = new Texture2D( GRID_DIMENSION + 1, GRID_DIMENSION + 1, GL_RED, GL_RED, GL_UNSIGNED_BYTE, GL_LINEAR_MIPMAP_NEAREST, GL_NEAREST);
            if(pFoamTexture[i] == nullptr)
            {
                Log("Memory Allocation Failed.");
                return false;
            }

            pFoamTexture[i]->GenerateMipmap();
        }

            // SHADER PROGRAM
        pWavesProgram = new ShaderProgram();
        if( pWavesProgram == nullptr)
        {
            Log("Memory Allocation Failed.");
            return false;
        }

        if( pWavesProgram->AddShaderFromFile( "resource/shaders/waves/vertex_shader.glsl", GL_VERTEX_SHADER) == false)
        {
            Log("Vertex Shader Failed.");
            return false;
        }

        if( pWavesProgram->AddShaderFromFile( "resource/shaders/waves/fragment_shader.glsl", GL_FRAGMENT_SHADER) == false)
        {
            Log("Fragment Shader Failed.");
            return false;
        }

        pWavesProgram->BindVertexAttributeLocation( "a_world_pos", ATTRIBUTE_INDEX::POSITION);
        pWavesProgram->BindVertexAttributeLocation( "a_normal", ATTRIBUTE_INDEX::NORMAL);
        pWavesProgram->BindVertexAttributeLocation( "a_tex_coord", ATTRIBUTE_INDEX::TEXCOORD2D);

        if( pWavesProgram->Build() == false)
        {
            Log("Shader Build Failed.");
            return false;
        }

            // clean up
        position_v.clear();
        normal_v.clear();
        texcoord_v.clear();
        indices_v.clear();


            // OpenCL Initialize
        cl_int ocl_err;

        ocl_wave_fbm_program = OpenCLUtil::CreateProgram("resource/opencl_kernel/waves.cl");
        if( ocl_wave_fbm_program == nullptr)
        {
            Log( "OpenCLUtil::CreateProgram() Failed.");
            return false;
        }

        ocl_wave_fbm_kernel = clCreateKernel( ocl_wave_fbm_program, "fbm", &ocl_err);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clCreateKernel() Failed.");
            return false;
        }

        ocl_position_buffer = clCreateFromGLBuffer( OpenCLUtil::GetContext(), CL_MEM_READ_WRITE, pGridVbo_Position->GetID(), &ocl_err);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clCreateFromGLBuffer() Failed.");
            return false;
        }

        ocl_normal_buffer = clCreateFromGLBuffer( OpenCLUtil::GetContext(), CL_MEM_READ_WRITE, pGridVbo_Normal->GetID(), &ocl_err);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clCreateFromGLBuffer() Failed.");
            return false;
        }

        ocl_wave_parameter = clCreateBuffer( OpenCLUtil::GetContext(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof( struct WaveProperty), &opencl_wave_property, &ocl_err);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clCreateBuffer() Failed.");
            return false;
        }

        for( int i = 0; i < 2; ++i)
        {
            ocl_foam_image[i] = clCreateFromGLTexture2D( OpenCLUtil::GetContext(), CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, pFoamTexture[i]->GetID(), &ocl_err);
            if( ocl_err != CL_SUCCESS)
            {
                Log("clCreateFromGLTexture2D() Failed.");
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Update()
     */
    void Update( float delta_time)
    {
        // variable declaration
        cl_int ocl_err;

        // code
        _time += delta_time;

        ocl_err = clSetKernelArg( ocl_wave_fbm_kernel, 0, sizeof( cl_mem), &ocl_position_buffer);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clSetKernelArg() Failed.");
            return;
        }

        ocl_err = clSetKernelArg( ocl_wave_fbm_kernel, 1, sizeof( cl_mem), &ocl_normal_buffer);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clSetKernelArg() Failed.");
            return;
        }

        ocl_err = clSetKernelArg( ocl_wave_fbm_kernel, 2, sizeof( cl_mem), &ocl_wave_parameter);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clSetKernelArg() Failed.");
            return;
        }

        ocl_err = clSetKernelArg( ocl_wave_fbm_kernel, 3, sizeof( cl_mem), &ocl_foam_image[read]);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clSetKernelArg() Failed.");
            return;
        }

        ocl_err = clSetKernelArg( ocl_wave_fbm_kernel, 4, sizeof( cl_mem), &ocl_foam_image[!read]);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clSetKernelArg() Failed.");
            return;
        }

        int tex_size = GRID_DIMENSION + 1;
        ocl_err = clSetKernelArg( ocl_wave_fbm_kernel, 5, sizeof( cl_float), &tex_size);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clSetKernelArg() Failed.");
            return;
        }

        ocl_err = clSetKernelArg( ocl_wave_fbm_kernel, 6, sizeof( cl_int), &vertex_count);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clSetKernelArg() Failed.");
            return;
        }

        ocl_err = clSetKernelArg( ocl_wave_fbm_kernel, 7, sizeof( cl_float), &_time);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clSetKernelArg() Failed.");
            return;
        }

        
            // Acquire GL object
        ocl_err = clEnqueueAcquireGLObjects( OpenCLUtil::GetCommandQueue(), 1, &ocl_position_buffer, 0, nullptr, nullptr);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clEnqueueAcquireGLObjects() Failed.");
            return;
        }

        ocl_err = clEnqueueAcquireGLObjects( OpenCLUtil::GetCommandQueue(), 1, &ocl_normal_buffer, 0, nullptr, nullptr);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clEnqueueAcquireGLObjects() Failed.");
            return;
        }

        ocl_err = clEnqueueAcquireGLObjects( OpenCLUtil::GetCommandQueue(), 2, ocl_foam_image, 0, nullptr, nullptr);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clEnqueueAcquireGLObjects() Failed.");
            return;
        }


        size_t global_work_size[] = { vertex_count };
        ocl_err = clEnqueueNDRangeKernel(
            OpenCLUtil::GetCommandQueue(), ocl_wave_fbm_kernel,
            1, nullptr, global_work_size, nullptr,
            0, nullptr, nullptr
        );
        if( ocl_err != CL_SUCCESS)
        {
            Log("clEnqueueNDRangeKernel() Failed.");
            return;
        }

            // Release GL object
        ocl_err = clEnqueueReleaseGLObjects( OpenCLUtil::GetCommandQueue(), 1, &ocl_position_buffer, 0, nullptr, nullptr);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clEnqueueReleaseGLObjects() Failed.");
            return;
        }

        ocl_err = clEnqueueReleaseGLObjects( OpenCLUtil::GetCommandQueue(), 1, &ocl_normal_buffer, 0, nullptr, nullptr);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clEnqueueReleaseGLObjects() Failed.");
            return;
        }

        ocl_err = clEnqueueReleaseGLObjects( OpenCLUtil::GetCommandQueue(), 2, ocl_foam_image, 0, nullptr, nullptr);
        if( ocl_err != CL_SUCCESS)
        {
            Log("clEnqueueReleaseGLObjects() Failed.");
            return;
        }

        read = !read;

        // clFinish( OpenCLUtil::GetCommandQueue());
    }

    /**
     * @brief Save()
     */
    void Save()
    {
#define WRITE_DATA_1( x)  \
        data.append( "\n" TO_STRING( x) "=" + std::to_string(x));

#define WRITE_DATA_2( x)  \
        data.append(("\n" TO_STRING( x) "=") + std::to_string(x[0]) + ", " + std::to_string(x[1]));

#define WRITE_DATA_3( x)  \
        data.append(("\n" TO_STRING( x) "=") + std::to_string(x[0]) + ", " + std::to_string(x[1]) + ", " + std::to_string(x[2]));

#define WRITE_DATA_4( x)  \
        data.append(("\n" TO_STRING( x) "=") + std::to_string(x[0]) + ", " + std::to_string(x[1]) + ", " + std::to_string(x[2]) + ", " + std::to_string(x[3]));

        // code
        std::string data;

        data.append( "[WAVE]");
        WRITE_DATA_1( u_vertex_wave_count);
        WRITE_DATA_1( u_wave_direction_seed);
        WRITE_DATA_1( u_wave_direction_seed_iter);
        WRITE_DATA_1( u_wave_frequency);
        WRITE_DATA_1( u_wave_frequency_mult);
        WRITE_DATA_1( u_wave_amplitude);
        WRITE_DATA_1( u_wave_amplitude_mult);
        WRITE_DATA_1( u_wave_initial_speed);
        WRITE_DATA_1( u_wave_speed_ramp);
        WRITE_DATA_1( u_wave_drag);
        WRITE_DATA_1( u_wave_height);
        WRITE_DATA_1( u_wave_max_peak);
        WRITE_DATA_1( u_wave_peak_offset);
        WRITE_DATA_1( u_normal_strength);
        data.append( "\n\n");

            // Light
        data.append( "[LIGHT]");
        WRITE_DATA_3( u_light_color);
        data.append( "\n\n");

            // Mateial
        data.append( "[MATERIAL]");
        WRITE_DATA_3( u_ambient);
        WRITE_DATA_3( u_diffuse_reflectance);
        WRITE_DATA_3( u_specular_reflectance);
        WRITE_DATA_1( u_specular_normal_strength);
        WRITE_DATA_1( u_shininess);
        data.append( "\n\n");

            // Fresnel
        data.append( "[Fresnel]");
        WRITE_DATA_1( u_fresnel_normal_strength);
        WRITE_DATA_1( u_fresnel_shininess);
        WRITE_DATA_1( u_fresnel_bias);
        WRITE_DATA_1( u_fresnel_strength);
        WRITE_DATA_3( u_fresnel_color);
        WRITE_DATA_1( u_use_environment_map);
        data.append( "\n\n");

            // Wave Tip
        data.append( "[WAVE_TIP]");
        WRITE_DATA_3( u_tip_color);
        WRITE_DATA_1( u_tip_attenuation);
        data.append( "\n\n");

        FileIO::Write( DATA_FILE_PATH, data);

#undef WRITE_DATA_1
#undef WRITE_DATA_2
#undef WRITE_DATA_3
#undef WRITE_DATA_4

    }

    /**
     * @brief RenderImGUI()
     */
    void RenderImGUI()
    {
        // code
        if( ImGui::TreeNode( "SceneWaves"))
        {
            ImGui::Text( "%f", _time);
            ImGui::Separator();

                // wave Parameter
            ImGui::SeparatorText( "Waves Parameter");
            {
                bool bChange = false;

                bChange |= ImGui::DragInt( "Wave Count", &u_vertex_wave_count, 1.0f, 0, 1024);

                bChange |= ImGui::DragFloat( "Direction Seed",          &u_wave_direction_seed,      0.001f, -100.0f, 100.0f);
                bChange |= ImGui::DragFloat( "Direction Seed Iterator", &u_wave_direction_seed_iter, 0.001f, -2048.0f, 2048.0f);
                bChange |= ImGui::DragFloat( "Frequency",               &u_wave_frequency,           0.001f, -128.0f, 128.0f);
                bChange |= ImGui::DragFloat( "Frequency Multiplier",    &u_wave_frequency_mult,      0.001f, -10.0f, 10.0f);
                bChange |= ImGui::DragFloat( "Amplitude",               &u_wave_amplitude,           0.001f, -1024.0f, 1024.0f);
                bChange |= ImGui::DragFloat( "Amplitude Multiplier",    &u_wave_amplitude_mult,      0.001f, -10.0f, 10.0f);
                bChange |= ImGui::DragFloat( "Initialial Speed",        &u_wave_initial_speed,       0.001f, -10.0f, 10.0f);
                bChange |= ImGui::DragFloat( "Speed Ramp",              &u_wave_speed_ramp,          0.001f, -10.0f, 10.0f);
                bChange |= ImGui::DragFloat( "Wave Drag",               &u_wave_drag,                0.001f, -10.0f, 10.0f);
                bChange |= ImGui::DragFloat( "Wave Height",             &u_wave_height,              0.001f, -512.0f, 512.0f);
                bChange |= ImGui::DragFloat( "Wave Max Peak",           &u_wave_max_peak,            0.001f, -1024.0f, 1024.0f);
                bChange |= ImGui::DragFloat( "Wave Peak Offset",        &u_wave_peak_offset,         0.001f, -512.0f, 512.0f);
                bChange |= ImGui::DragFloat( "Normal Strength",         &u_normal_strength,          0.001f, -10.0f, 10.0f);

                if( bChange)
                {
                    opencl_wave_property.u_wave_direction_seed_param[0] = u_wave_direction_seed;
                    opencl_wave_property.u_wave_direction_seed_param[1] = u_wave_direction_seed_iter;
                    opencl_wave_property.u_wave_frequency_param[0] = u_wave_frequency;
                    opencl_wave_property.u_wave_frequency_param[1] = u_wave_frequency_mult;
                    opencl_wave_property.u_wave_amplitude_param[0] = u_wave_amplitude;
                    opencl_wave_property.u_wave_amplitude_param[1] = u_wave_amplitude_mult;
                    opencl_wave_property.u_wave_speed_param[0] = u_wave_initial_speed;
                    opencl_wave_property.u_wave_speed_param[1] = u_wave_speed_ramp;
                    opencl_wave_property.u_wave_drag = u_wave_drag;
                    opencl_wave_property.u_wave_height = u_wave_height;
                    opencl_wave_property.u_wave_max_peak = u_wave_max_peak;
                    opencl_wave_property.u_wave_peak_offset = u_wave_peak_offset;
                    opencl_wave_property.u_wave_count = u_vertex_wave_count;
                    // _time = 0.0f;

                    cl_int ocl_err = clEnqueueWriteBuffer( OpenCLUtil::GetCommandQueue(), ocl_wave_parameter, CL_TRUE, 0, sizeof( struct WaveProperty), &opencl_wave_property, 0, nullptr, nullptr);
                    if( ocl_err != CL_SUCCESS)
                    {
                        Log( "clEnqueueWriteBuffer() Failed.");
                    }
                }
            }

                // light parameter
            ImGui::SeparatorText( "Light Parameter");
            ImGui::ColorEdit3( "Light Color", &u_light_color[0], ImGuiColorEditFlags_Float);

                // Mateial
            ImGui::SeparatorText( "Material Parameter");
            ImGui::ColorEdit3( "Ambient", &u_ambient[0], ImGuiColorEditFlags_Float);
            ImGui::ColorEdit3( "Diffuse Reflectance", &u_diffuse_reflectance[0], ImGuiColorEditFlags_Float);
            ImGui::ColorEdit3( "Specular Reflectance", &u_specular_reflectance[0], ImGuiColorEditFlags_Float);
            ImGui::DragFloat( "Specular Normal Strength", &u_specular_normal_strength, 0.01f, 0.0f, 10.0f);
            ImGui::DragFloat( "Shininess", &u_shininess, 0.01f, 0.0f, 256.0f);

                // Fresnel
            ImGui::SeparatorText( "Fresnel Parameter");
            ImGui::DragFloat( "Fresnel Normal Strength", &u_fresnel_normal_strength, 0.001f, 0.0f, 10.0f);
            ImGui::DragFloat( "Fresnel Shininess", &u_fresnel_shininess, 0.01f, 0.0f, 256.0f);
            ImGui::DragFloat( "Fresnel Bias", &u_fresnel_bias, 0.001f, 0.0f, 10.0f);
            ImGui::DragFloat( "Fresnel Strength", &u_fresnel_strength, 0.001f, 0.0f, 10.0f);
            ImGui::ColorEdit3( "Fresnel Color", &u_fresnel_color[0], ImGuiColorEditFlags_Float);

            bool bUseEnvMap = (bool) u_use_environment_map;
            if( ImGui::Checkbox( "Use Env Map", &bUseEnvMap))
            {
                u_use_environment_map = !u_use_environment_map;
            }

                // Wave Tip
            ImGui::SeparatorText( "Wave Tip Parameter");
            ImGui::ColorEdit3( "Tip Color", &u_tip_color[0], ImGuiColorEditFlags_Float);
            ImGui::DragFloat( "Tip Attenuation", &u_tip_attenuation, 0.001f, -100.0f, 100.0f);

            if( ImGui::Button("Save"))
            {
                Save();
            }

            ImGui::TreePop();
        }
    }

    /**
     * @brief Render()
     */
    void Render(
        vmath::mat4 view_matrix, vmath::mat4 projection_matrix, vmath::vec3& camera_position,
        vmath::vec3& sun_direction, vmath::vec3& sun_color,
        TextureCubeMap *p_env_map
    )
    {
        // code
        if( !p_env_map)
        {
            Log("Error: Invalide Parameter.");
            return;
        }

        pWavesProgram->Bind();
            pWavesProgram->SetMatrix4x4( "u_projection", GL_FALSE, projection_matrix);
            pWavesProgram->SetMatrix4x4( "u_view", GL_FALSE, view_matrix);
            pWavesProgram->SetUniformFloat3( "u_camera_position", camera_position[0], camera_position[1], camera_position[2]);

                // sun
            pWavesProgram->SetUniformFloat3( "u_sun_direction", sun_direction[0], sun_direction[1], sun_direction[2]);
            pWavesProgram->SetUniformFloat3( "u_sun_color", sun_color[0], sun_color[1], sun_color[2]);

                // light
            pWavesProgram->SetUniformFloat3( "u_light_color", u_light_color[0], u_light_color[1], u_light_color[2]);

                // Wave
            pWavesProgram->SetUniformFloat( "u_normal_strength",              u_normal_strength);

                // Material
            pWavesProgram->SetUniformFloat3( "u_ambient", u_ambient[0], u_ambient[1], u_ambient[2]);
            pWavesProgram->SetUniformFloat3( "u_diffuse_reflectance", u_diffuse_reflectance[0], u_diffuse_reflectance[1], u_diffuse_reflectance[2]);
            pWavesProgram->SetUniformFloat3( "u_specular_reflectance", u_specular_reflectance[0], u_specular_reflectance[1], u_specular_reflectance[2]);
            pWavesProgram->SetUniformFloat( "u_specular_normal_strength", u_specular_normal_strength);
            pWavesProgram->SetUniformFloat( "u_shininess", u_shininess);

                // frasnel
            pWavesProgram->SetUniformFloat4( "u_fresnel_parameter", u_fresnel_normal_strength, u_fresnel_shininess, u_fresnel_bias, u_fresnel_strength);
            pWavesProgram->SetUniformFloat3( "u_fresnel_color", u_fresnel_color[0], u_fresnel_color[1], u_fresnel_color[2]);
            pWavesProgram->SetUniformInt( "u_use_environment_map", u_use_environment_map);
            
                // wave tip
            pWavesProgram->SetUniformFloat3( "u_tip_color", u_tip_color[0], u_tip_color[1], u_tip_color[2]);
            pWavesProgram->SetUniformFloat( "u_tip_attenuation", u_tip_attenuation);

            pWavesProgram->SetUniformInt( "u_sky_map", 0);
            pWavesProgram->SetUniformInt( "u_foam_texture", 1);

                // generate mipmap
            pFoamTexture[read]->GenerateMipmap();

            p_env_map->Bind(0);
            pFoamTexture[read]->Bind(1);

            pGridVao->Bind();
                glDrawElements( GL_TRIANGLES, indices_count, GL_UNSIGNED_INT, nullptr);
            pGridVao->Unbind();

            p_env_map->Unbind(0);
            pFoamTexture[read]->Unbind(1);

        pWavesProgram->Unbind();
    }

    /**
     * @brief Uninitialize()
     */
    void Uninitialize()
    {
        // code
        CL_OBJECT_RELEASE( ocl_wave_fbm_program, clReleaseProgram);
        CL_OBJECT_RELEASE( ocl_wave_fbm_kernel, clReleaseKernel);
        CL_OBJECT_RELEASE( ocl_position_buffer, clReleaseMemObject);
        CL_OBJECT_RELEASE( ocl_normal_buffer, clReleaseMemObject);
        CL_OBJECT_RELEASE( ocl_wave_parameter, clReleaseMemObject);
        CL_OBJECT_RELEASE( ocl_foam_image[0], clReleaseMemObject);
        CL_OBJECT_RELEASE( ocl_foam_image[1], clReleaseMemObject);

        if( pWavesProgram)
        {
            delete pWavesProgram;
            pWavesProgram = nullptr;
        }

        if( pGridVao)
        {
            delete pGridVao;
            pGridVao = nullptr;
        }

        if( pGridVbo_Position)
        {
            delete pGridVbo_Position;
            pGridVbo_Position = nullptr;
        }

        if( pGridVbo_Normal)
        {
            delete pGridVbo_Normal;
            pGridVbo_Normal = nullptr;
        }
        
        if( pGridVbo_Texcoord)
        {
            delete pGridVbo_Texcoord;
            pGridVbo_Texcoord = nullptr;
        }

        if( pGridIbo)
        {
            delete pGridIbo;
            pGridIbo = nullptr;
        }

        if( pFoamTexture[0])
        {
            pFoamTexture[0]->Release();
            delete pFoamTexture[0];
            pFoamTexture[0] = nullptr;
        }

        if( pFoamTexture[1])
        {
            pFoamTexture[1]->Release();
            delete pFoamTexture[1];
            pFoamTexture[1] = nullptr;
        }
    }

} // namespace Ocean
